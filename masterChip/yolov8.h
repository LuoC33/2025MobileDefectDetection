#ifndef _YOLOV8_H
#define _YOLOV8_H

#include "utils.h"
#include "ai_base.h"


/**
 * @brief Yolov8
 * 主要封装了对于每一帧图片，从预处理、运行到后处理给出结果的过程
 */
class Yolov8 : public AIBase
{
    public:

    /**
    * @brief Yolov8构造函数，加载kmodel,并初始化kmodel输入、输出
    * @param kmodel_file kmodel文件路径
    * @param conf_thres 多目标检测conf_thres
    * @param nms_thres   多目标检测nms阈值
    * @param debug_mode
    * @return None
    */
    Yolov8(char* task_type,char* task_mode,char *kmodel_file, float conf_thres, float nms_thres,float mask_thres,std::vector<std::string> labels,FrameSize image_wh, int debug_mode = 0);
    
    /**
    * @brief Yolov8析构函数
    * @return None
    */
    ~Yolov8();

    void pre_process(runtime_tensor &input_tensor);

    /**
    * @brief kmodel推理
    * @return None
    */
    void inference();

    void post_process(std::vector<YOLOBbox> &yolo_results);

    void draw_results(cv::Mat &draw_frame,std::vector<YOLOBbox> &yolo_results);

    private:

    void yolov8_nms(std::vector<YOLOBbox> &bboxes,  float confThreshold, float nmsThreshold, std::vector<int> &indices);
    
    float yolov8_iou_calculate(cv::Rect &rect1, cv::Rect &rect2);

    float fast_exp(float x);

    float sigmoid(float x);

    std::unique_ptr<ai2d_builder> ai2d_builder_; // ai2d构建器
    runtime_tensor model_input_tensor_; // ai2d输出tensor

    char* task_type_;
    char* task_mode_;
    // 标签列表
    std::vector<std::string> labels_;
    int label_num_;
    std::vector<cv::Scalar> colors;
    FrameSize image_wh_;
    FrameSize input_wh_;
    // 置信度阈值
    float conf_thres_;
    // nms阈值
    float nms_thres_;
    // 分割任务使用的mask阈值
    float mask_thres_;
    // 检测框的总数
    int box_num_;
    int max_box_num_;
    // 每个检测框的特征维度
    int box_feature_len_;
    // kmodel的输出处理结果
    float *output_det_;
    int debug_mode_;
};
#endif