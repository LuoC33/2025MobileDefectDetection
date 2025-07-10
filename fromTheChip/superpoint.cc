#include "superpoint.h"

SuperPoint::SuperPoint(const char* kmodel_file, float conf_thres, float nms_radius, FrameSize image_wh, int debug_mode)
    : AIBase(kmodel_file, "SuperPoint", debug_mode),
      conf_thres_(conf_thres),
      nms_radius_(nms_radius),
      image_wh_(image_wh),
      debug_mode_(debug_mode)
{
    // 获取模型输入尺寸
    input_wh_ = {input_shapes_[0][3], input_shapes_[0][2]};
    
    // 计算预处理参数
    ratio_ = std::min((float)input_wh_.width / image_wh_.width, 
                      (float)input_wh_.height / image_wh_.height);
    
    int new_w = (int)(ratio_ * image_wh_.width);
    int new_h = (int)(ratio_ * image_wh_.height);
    
    pad_w_ = (input_wh_.width - new_w) / 2.0f;
    pad_h_ = (input_wh_.height - new_h) / 2.0f;
    
    // 初始化AI2D构建器
    Utils::padding_resize_one_side_set(image_wh_, input_wh_, ai2d_builder_, cv::Scalar(0));
    
    // 获取模型输入tensor
    model_input_tensor_ = get_input_tensor(0);
}

SuperPoint::~SuperPoint()
{
}

void SuperPoint::pre_process(runtime_tensor &input_tensor)
{
    ai2d_builder_->invoke(input_tensor, model_input_tensor_).expect("AI2D invoke failed");
}

void SuperPoint::inference()
{
    this->run();
    this->get_output();
}

void SuperPoint::post_process(std::vector<cv::Point2f> &keypoints, cv::Mat &descriptors)
{
    ScopedTiming st("SuperPoint::post_process", debug_mode_);
    keypoints.clear();
    
    // 获取模型输出
    int8_t* semi_data = reinterpret_cast<int8_t*>(p_outputs_[0]);  // [1,65,H/8,W/8]
    int8_t* desc_data = reinterpret_cast<int8_t*>(p_outputs_[1]);  // [1,256,H/8,W/8]
    
    const int heatmap_h = input_shapes_[0][2] / 8;
    const int heatmap_w = input_shapes_[0][3] / 8;
    
    // 提取关键点
    std::vector<cv::Point2f> raw_keypoints;
    std::vector<float> scores;
    
    for (int y = 0; y < heatmap_h; ++y) {
        for (int x = 0; x < heatmap_w; ++x) {
            // 获取65通道中前64个通道的最大值
            int8_t max_val = -128;
            for (int c = 0; c < 64; ++c) {
                int8_t val = semi_data[c * heatmap_h * heatmap_w + y * heatmap_w + x];
                max_val = std::max(max_val, val);
            }
            
            float prob = static_cast<float>(max_val) / 127.0f;
            
            if (prob > conf_thres_) {
                // 计算原始图像坐标（考虑padding和缩放）
                float orig_x = (x + 0.5f) * 8 / ratio_ - pad_w_;
                float orig_y = (y + 0.5f) * 8 / ratio_ - pad_h_;
                
                // 确保坐标在有效范围内
                if (orig_x >= 0 && orig_x < image_wh_.width && 
                    orig_y >= 0 && orig_y < image_wh_.height) {
                    raw_keypoints.emplace_back(orig_x, orig_y);
                    scores.push_back(prob);
                }
            }
        }
    }
    
    // 非极大值抑制
    if (!raw_keypoints.empty()) {
        // 创建网格进行NMS
        const int grid_size = static_cast<int>(nms_radius_);
        const int grid_width = (image_wh_.width + grid_size - 1) / grid_size;
        const int grid_height = (image_wh_.height + grid_size - 1) / grid_size;
        
        std::vector<std::vector<int>> grid(grid_width * grid_height);
        
        // 将关键点分配到网格
        for (size_t i = 0; i < raw_keypoints.size(); ++i) {
            const auto& kp = raw_keypoints[i];
            int grid_x = static_cast<int>(kp.x / grid_size);
            int grid_y = static_cast<int>(kp.y / grid_size);
            grid[grid_y * grid_width + grid_x].push_back(i);
        }
        
        // 在每个网格中选择得分最高的关键点
        for (const auto& cell : grid) {
            if (cell.empty()) continue;
            
            int best_idx = cell[0];
            float best_score = scores[best_idx];
            
            for (size_t i = 1; i < cell.size(); ++i) {
                if (scores[cell[i]] > best_score) {
                    best_idx = cell[i];
                    best_score = scores[best_idx];
                }
            }
            
            keypoints.push_back(raw_keypoints[best_idx]);
        }
    }
    
    // 提取描述符
    if (!keypoints.empty()) {
        descriptors = cv::Mat::zeros(keypoints.size(), 256, CV_32F);
        
        for (size_t i = 0; i < keypoints.size(); ++i) {
            const auto& kp = keypoints[i];
            
            // 计算在描述符图中的位置
            float desc_x = (kp.x + pad_w_) * ratio_ / 8.0f;
            float desc_y = (kp.y + pad_h_) * ratio_ / 8.0f;
            
            int x0 = static_cast<int>(desc_x);
            int y0 = static_cast<int>(desc_y);
            
            // 确保在有效范围内
            x0 = std::max(0, std::min(x0, heatmap_w - 2));
            y0 = std::max(0, std::min(y0, heatmap_h - 2));
            
            // 双线性插值
            float wx = desc_x - x0;
            float wy = desc_y - y0;
            
            for (int c = 0; c < 256; ++c) {
                float v00 = desc_data[c * heatmap_h * heatmap_w + y0 * heatmap_w + x0];
                float v01 = desc_data[c * heatmap_h * heatmap_w + y0 * heatmap_w + (x0 + 1)];
                float v10 = desc_data[c * heatmap_h * heatmap_w + (y0 + 1) * heatmap_w + x0];
                float v11 = desc_data[c * heatmap_h * heatmap_w + (y0 + 1) * heatmap_w + (x0 + 1)];
                
                float val = (1 - wy) * ((1 - wx) * v00 + wx * v01) + 
                            wy * ((1 - wx) * v10 + wx * v11);
                
                descriptors.at<float>(i, c) = val / 127.0f;  // 归一化
            }
        }
        for (int i = 0; i < descriptors.rows; ++i) {
            cv::Mat row = descriptors.row(i);
            float norm = cv::norm(row, cv::NORM_L2);
            if (norm > 1e-6) {
                row /= norm;
            }

            row.copyTo(descriptors.row(i));
        }

    }
    
    if (debug_mode_ > 1) {
        printf("Detected %zu keypoints\n", keypoints.size());
            // 新增关键点坐标输出
        // printf("Keypoint coordinates (in original image space):\n");
        // for (size_t i = 0; i < keypoints.size(); ++i) {
        //     printf("  KP %4zu: (%.2f, %.2f)\n", 
        //        i, keypoints[i].x, keypoints[i].y);
        // }
    }
}


void SuperPoint::draw_results(cv::Mat &draw_frame, const std::vector<cv::Point2f> &keypoints) {
    ScopedTiming st("SuperPoint::draw_results", debug_mode_);
    
    int disp_w = draw_frame.cols;
    int disp_h = draw_frame.rows;
    float scale_x = (float)disp_w / image_wh_.width;
    float scale_y = (float)disp_h / image_wh_.height;

    //绘制关键点（绿色实心圆）
    for (const auto &kp : keypoints) {
        int x = static_cast<int>(kp.x * scale_x);
        int y = static_cast<int>(kp.y * scale_y);
        if (x >= 0 && x < disp_w && y >= 0 && y < disp_h) {
            cv::circle(draw_frame, cv::Point(x, y), 
                      30, cv::Scalar(0, 255, 0 ,255), -1);
        }
    }

    //调试信息
    if (debug_mode_ > 1) {
        std::string text = "Keypoints: " + std::to_string(keypoints.size());
        cv::putText(draw_frame, text, cv::Point(10, 30), 
                    cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 255, 0,255), 2);
    }
}

runtime_tensor SuperPoint::get_model_input_tensor() {
    return model_input_tensor_;
}

std::vector<int> SuperPoint::get_expected_input_shape() {
    return {1, 1, 320, 320};
}
