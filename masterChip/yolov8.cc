#include "yolov8.h"

Yolov8::Yolov8(char* task_type, char* task_mode, char *kmodel_file, float conf_thres,float nms_thres,float mask_thres,std::vector<std::string> labels, FrameSize image_wh,int debug_mode)
:AIBase(kmodel_file,"Yolov8", debug_mode)
{
    task_type_=task_type;
    task_mode_=task_mode;
    conf_thres_=conf_thres;
    nms_thres_=nms_thres;
    mask_thres_=mask_thres;
    image_wh_=image_wh;
    input_wh_={input_shapes_[0][3], input_shapes_[0][2]};
    labels_=labels;
    label_num_=labels_.size();
    colors=getColorsForClasses(label_num_);
    max_box_num_=50;
    box_num_=((input_wh_.width/8)*(input_wh_.height/8)+(input_wh_.width/16)*(input_wh_.height/16)+(input_wh_.width/32)*(input_wh_.height/32));
    debug_mode_=debug_mode;
    model_input_tensor_=get_input_tensor(0);
    if(strcmp(task_type_,"classify")==0){
        Utils::center_crop_resize_set(image_wh_,input_wh_,ai2d_builder_);
    }
    else if(strcmp(task_type_,"detect")==0){
        box_feature_len_=label_num_+4;
        Utils::padding_resize_one_side_set(image_wh_,input_wh_,ai2d_builder_, cv::Scalar(114, 114, 114));
    }
    else if(strcmp(task_type_,"segment")==0){
        box_feature_len_=label_num_+4+32;
        Utils::padding_resize_one_side_set(image_wh_,input_wh_,ai2d_builder_, cv::Scalar(114, 114, 114));
    }
    else{
        std::cerr << "不支持该任务类型: " << task_type_ << std::endl;
        exit(EXIT_FAILURE);
    }

}
    
Yolov8::~Yolov8()
{
}

void Yolov8::pre_process(runtime_tensor &input_tensor)
{
    ScopedTiming st("Yolov8::pre_process", debug_mode_);
    ai2d_builder_->invoke(input_tensor,model_input_tensor_).expect("error occurred in ai2d running");
}

void Yolov8::inference()
{
    this->run();
    this->get_output();
}

void Yolov8::post_process(std::vector<YOLOBbox> &yolo_results)
{
    ScopedTiming st("Yolov8::post_process", debug_mode_);
    yolo_results.clear();
    if(strcmp(task_type_,"classify")==0){
        float* output0 = p_outputs_[0];
        YOLOBbox res;
        if(label_num_ > 2){
            float sum = 0.0;
            for (int i = 0; i < label_num_; i++){
                sum += exp(output0[i]);
            }
            int max_index;
            for (int i = 0; i < label_num_; i++)
            {
                output0[i] = exp(output0[i]) / sum;
            }
            max_index = std::max_element(output0,output0+label_num_) - output0; 
            if (output0[max_index] >= conf_thres_)
            {
                res.index = max_index;
                res.confidence = output0[max_index];
                yolo_results.push_back(res);
            }
        }
        else
        {
            float pred = sigmoid(output0[0]);
            if (pred > conf_thres_)
            {
                res.index = 1;
                res.confidence = pred;
            }
            else{
                res.index = 0;
                res.confidence = 1-pred;
            }
            yolo_results.push_back(res);
        }
    }
    else if(strcmp(task_type_,"detect")==0){
        float ratiow = (float)input_wh_.width / image_wh_.width;
        float ratioh = (float)input_wh_.height / image_wh_.height;
        float ratio = ratiow < ratioh ? ratiow : ratioh;
        float *output_det = new float[box_num_ * box_feature_len_];
        // 模型推理结束后，进行后处理
        float* output0= p_outputs_[0];
        // 将输出数据排布从[label_num_+4,(w/8)*(h/8)+(w/16)*(h/16)+(w/32)*(h/32)]调整为[(w/8)*(h/8)+(w/16)*(h/16)+(w/32)*(h/32),label_num_+4],方便后续处理
        for(int r = 0; r < box_num_; r++)
        {
            for(int c = 0; c < box_feature_len_; c++)
            {
                output_det[r*box_feature_len_ + c] = output0[c*box_num_ + r];
            }
        }
        for(int i=0;i<box_num_;i++){
            float* vec=output_det+i*box_feature_len_;
            float box[4]={vec[0],vec[1],vec[2],vec[3]};
            float* class_scores=vec+4;
            float* max_class_score_ptr=std::max_element(class_scores,class_scores+label_num_);
            float score=*max_class_score_ptr;
            int max_class_index = max_class_score_ptr - class_scores; // 计算索引
            if(score>conf_thres_){
                YOLOBbox bbox;
                float x_=box[0]/ratio*1.0;
                float y_=box[1]/ratio*1.0;
                float w_=box[2]/ratio*1.0;
                float h_=box[3]/ratio*1.0;
                int x=int(MAX(x_-0.5*w_,0));
                int y=int(MAX(y_-0.5*h_,0));
                int w=int(w_);
                int h=int(h_);
                if (w <= 0 || h <= 0) { continue; }
                bbox.box=cv::Rect(x,y,w,h);
                bbox.confidence=score;
                bbox.index=max_class_index;
                yolo_results.push_back(bbox);
            }

        }
        //执行非最大抑制以消除具有较低置信度的冗余重叠框（NMS）
        std::vector<int> nms_result;
        yolov8_nms(yolo_results, conf_thres_, nms_thres_, nms_result);
        delete[] output_det;
    }
    else if(strcmp(task_type_,"segment")==0){
        float ratiow = input_wh_.width / (image_wh_.width*1.0);
        float ratioh = input_wh_.height / (image_wh_.height*1.0);
        float ratio = ratiow < ratioh ? ratiow : ratioh;
        int new_w=int(image_wh_.width*ratio);
        int new_h=int(image_wh_.height*ratio);
        int pad_w=input_wh_.width-new_w>0?input_wh_.width-new_w:0;
        int pad_h=input_wh_.height-new_h>0?input_wh_.height-new_h:0;
        // std::vector<YOLOBbox> bboxes;
        float *output_det = new float[box_num_ * box_feature_len_];
        // 模型推理结束后，进行后处理
        float* output0= p_outputs_[0];
        // 将输出数据排布从[label_num_+4+32,(w/8)*(h/8)+(w/16)*(h/16)+(w/32)*(h/32)]调整为[(w/8)*(h/8)+(w/16)*(h/16)+(w/32)*(h/32),label_num_+4+32],方便后续处理
        for(int r = 0; r < box_num_; r++)
        {
            for(int c = 0; c < box_feature_len_; c++)
            {
                output_det[r*box_feature_len_ + c] = output0[c*box_num_ + r];
            }
        }

        float* output1=p_outputs_[1];
        int mask_w=input_wh_.width/4;
        int mask_h=input_wh_.height/4;
        cv::Mat protos=cv::Mat(32,mask_w*mask_h,CV_32FC1,output1);
        for(int i=0;i<box_num_;i++){
            float* vec=output_det+i*box_feature_len_;
            float box[4]={vec[0],vec[1],vec[2],vec[3]};
            float* class_scores=vec+4;
            float* max_class_score_ptr=std::max_element(class_scores,class_scores+label_num_);
            float score=*max_class_score_ptr;
            int max_class_index = max_class_score_ptr - class_scores; // 计算索引
            if(score>conf_thres_){
                YOLOBbox bbox;
                float x_=box[0]/ratio*1.0;
                float y_=box[1]/ratio*1.0;
                float w_=box[2]/ratio*1.0;
                float h_=box[3]/ratio*1.0;
                int x=int(MAX(x_-0.5*w_,0));
                int y=int(MAX(y_-0.5*h_,0));
                int w=int(w_);
                int h=int(h_);
                if (w <= 0 || h <= 0) { continue; }
                bbox.box=cv::Rect(x,y,w,h);
                bbox.confidence=score;
                bbox.index=max_class_index;
                cv::Mat mask_(1,32,CV_32F,vec+label_num_+4);
                cv::Mat mask_box=(mask_*protos);
                cv::Mat mask_box_(mask_h,mask_w,CV_32FC1,mask_box.data);
                cv::Rect roi(0,0,int(mask_w-int(pad_w*((mask_w*1.0)/input_wh_.width))),int(mask_h-int(pad_h*((mask_h*1.0)/input_wh_.height))));
                cv::Mat dest,mask_res;
                cv::exp(-mask_box_,dest);
                dest=1.0/(1.0+dest);
                dest=dest(roi);
                bbox.mask=dest;
                const int area = w * h;
                float ratio = 0.0f;
                if (area > 0) {
                    // 二值化掩码
                    cv::Mat binary_mask;
                    cv::threshold(dest, binary_mask, 0.5f, 1.0f, cv::THRESH_BINARY);
    
                    int mask_pixels = cv::countNonZero(binary_mask);

                    ratio = static_cast<float>(mask_pixels) / area;
                }
                bbox.ratio = ratio;
                yolo_results.push_back(bbox);
            }

        }
        //执行非最大抑制以消除具有较低置信度的冗余重叠框（NMS）
        std::vector<int> nms_result;
        yolov8_nms(yolo_results, conf_thres_, nms_thres_, nms_result);
        delete[] output_det;
    }
}

void Yolov8::draw_results(cv::Mat &draw_frame,std::vector<YOLOBbox> &yolo_results)
{
    ScopedTiming st("Yolov8::draw_results", debug_mode_);
    if(strcmp(task_type_,"classify")==0){
        if(yolo_results.size()>0){
            string text=labels_[yolo_results[0].index]+" score:"+std::to_string(yolo_results[0].confidence);
            cv::putText(draw_frame, text, cv::Point(50,50), cv::FONT_HERSHEY_DUPLEX, 1, colors[yolo_results[0].index], 2, 0);
        }
    }
    else if(strcmp(task_type_,"detect")==0){
        int w_=draw_frame.cols;
        int h_=draw_frame.rows;
        int res_size=MIN(yolo_results.size(),max_box_num_);
        for(int i=0;i<res_size;i++){
            YOLOBbox box_=yolo_results[i];
            cv::Rect box=box_.box;
            int idx=box_.index;
            float score=box_.confidence;
            int x=int(box.x*float(w_)/image_wh_.width);
            int y=int(box.y*float(h_)/image_wh_.height);
            int w=int(box.width*float(w_)/image_wh_.width);
            int h=int(box.height*float(h_)/image_wh_.height);
            int x_right = x + w;
            int y_bottom = y + h;
            if (x_right > w_)
            {
                w = w_ - x;
            }
            if (y_bottom > h_)
            {
                h = h_ - y;
            }
            cv::Rect new_box(x,y,w,h);
            cv::rectangle(draw_frame, new_box, colors[idx], 2, 8);
            cv::putText(draw_frame, labels_[idx]+" "+std::to_string(score), cv::Point(MIN(new_box.x + 5,w_), MAX(new_box.y - 10,0)), cv::FONT_HERSHEY_DUPLEX, 1, colors[idx], 2, 0);
        }
    }
    else if(strcmp(task_type_,"segment")==0){
        int w_=draw_frame.cols;
        int h_=draw_frame.rows;
        int res_size=MIN(yolo_results.size(),max_box_num_);
        for(int i=0;i<res_size;i++){
            YOLOBbox box_=yolo_results[i];
            cv::Rect box=box_.box;
            int idx=box_.index;
            float score=box_.confidence;
            int x=int(box.x*float(w_)/image_wh_.width);
            int y=int(box.y*float(h_)/image_wh_.height);
            int w=int(box.width*float(w_)/image_wh_.width);
            int h=int(box.height*float(h_)/image_wh_.height);
            int x_right = x + w;
            int y_bottom = y + h;
            if (x_right > w_)
            {
                w = w_ - x;
            }
            if (y_bottom > h_)
            {
                h = h_ - y;
            }
            cv::Rect new_box(x,y,w,h);
            cv::rectangle(draw_frame, new_box, colors[idx], 2, 8);
            cv::putText(draw_frame, labels_[idx]+" "+std::to_string(score), cv::Point(MIN(new_box.x + 5,w_), MAX(new_box.y - 10,0)), cv::FONT_HERSHEY_DUPLEX, 1, colors[idx], 2, 0);
            
            cv::Mat mask=box_.mask;
            cv::Mat mask_d;
            cv::resize(mask,mask_d,cv::Size(w_,h_),cv::INTER_NEAREST);
            mask_d=mask_d(new_box) > mask_thres_;
            draw_frame(new_box).setTo(colors[idx],mask_d);
        }
    }
}

void Yolov8::yolov8_nms(std::vector<YOLOBbox> &bboxes,  float confThreshold, float nmsThreshold, std::vector<int> &indices)
{	
    std::sort(bboxes.begin(), bboxes.end(), [](YOLOBbox &a, YOLOBbox &b) { return a.confidence > b.confidence; });
    int updated_size = bboxes.size();
    for (int i = 0; i < updated_size; i++) {
        if (bboxes[i].confidence < confThreshold)
            continue;
        indices.push_back(i);
        // 这里使用移除冗余框，而不是 erase 操作，减少内存移动的开销
        for (int j = i + 1; j < updated_size;) {
            float iou = yolov8_iou_calculate(bboxes[i].box, bboxes[j].box);
            if (iou > nmsThreshold) {
                bboxes[j].confidence = -1;  // 设置为负值，后续不会再计算其IOU
            }
            j++;
        }
    }

    // 移除那些置信度小于0的框
    bboxes.erase(std::remove_if(bboxes.begin(), bboxes.end(), [](YOLOBbox &b) { return b.confidence < 0; }), bboxes.end());
}

float Yolov8::yolov8_iou_calculate(cv::Rect &rect1, cv::Rect &rect2)
{
    int xx1, yy1, xx2, yy2;
 
	xx1 = std::max(rect1.x, rect2.x);
	yy1 = std::max(rect1.y, rect2.y);
	xx2 = std::min(rect1.x + rect1.width - 1, rect2.x + rect2.width - 1);
	yy2 = std::min(rect1.y + rect1.height - 1, rect2.y + rect2.height - 1);
 
	int insection_width, insection_height;
	insection_width = std::max(0, xx2 - xx1 + 1);
	insection_height = std::max(0, yy2 - yy1 + 1);
 
	float insection_area, union_area, iou;
	insection_area = float(insection_width) * insection_height;
	union_area = float(rect1.width*rect1.height + rect2.width*rect2.height - insection_area);
	iou = insection_area / union_area;

	return iou;
}

float Yolov8::fast_exp(float x)
{
    union {
        uint32_t i;
        float f;
    } v{};
    v.i = (1 << 23) * (1.4426950409 * x + 126.93490512f);
    return v.f;
}

float Yolov8::sigmoid(float x)
{
    return 1.0f / (1.0f + fast_exp(-x));
}