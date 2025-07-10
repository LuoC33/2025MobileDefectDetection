#ifndef _SUPERPOINT_H
#define _SUPERPOINT_H

#include "ai_base.h"
#include "utils.h"

/**
 * @brief SuperPoint特征点检测和描述符提取
 * 封装了SuperPoint模型的前处理、推理和后处理流程
 */
class SuperPoint : public AIBase
{
public:
    /**
     * @brief SuperPoint构造函数
     * @param kmodel_file kmodel文件路径
     * @param conf_thres 关键点置信度阈值
     * @param nms_radius 非极大值抑制半径
     * @param debug_mode 调试模式
     */
    SuperPoint(const char* kmodel_file, float conf_thres, float nms_radius, FrameSize image_wh, int debug_mode = 0);
    
    /**
     * @brief SuperPoint析构函数
     */
    ~SuperPoint();

    /**
     * @brief 图像预处理
     * @param input_tensor 输入图像tensor
     */
    void pre_process(runtime_tensor &input_tensor);

    /**
     * @brief 执行模型推理
     */
    void inference();

    /**
     * @brief 后处理，提取关键点和描述符
     * @param keypoints 输出关键点
     * @param descriptors 输出描述符
     */
    void post_process(std::vector<cv::Point2f> &keypoints, cv::Mat &descriptors);

    void draw_results(cv::Mat &draw_frame, const std::vector<cv::Point2f> &keypoints);

    runtime_tensor get_model_input_tensor();

    std::vector<int> get_expected_input_shape();

private:
    std::unique_ptr<ai2d_builder> ai2d_builder_; // ai2d构建器
    runtime_tensor model_input_tensor_;         // ai2d输出tensor

    FrameSize image_wh_;        // 原始图像尺寸
    FrameSize input_wh_;        // 模型输入尺寸
    float conf_thres_;          // 关键点置信度阈值
    float nms_radius_;          // 非极大值抑制半径
    float pad_w_;               // 水平padding
    float pad_h_;               // 垂直padding
    float ratio_;               // 缩放比例
    int debug_mode_;            // 调试模式
};

#endif
