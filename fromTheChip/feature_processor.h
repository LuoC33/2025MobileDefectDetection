#ifndef FEATURE_PROCESSOR_H
#define FEATURE_PROCESSOR_H

#include <vector>
#include <opencv2/core.hpp>
#include "pipeline.h"
#include "ORBExtractor.h"
#include "superpoint.h"
#include "utils.h"
#include <nncase/runtime/runtime_tensor.h>
#include <nncase/runtime/runtime_op_utility.h>
#include <opencv2/imgcodecs.hpp>

// 通用配置引用
extern GeneralConfig general_config;

struct ObjectFeatures {
    YOLOBbox bbox;
    std::vector<cv::Point2f> keypoints;
    cv::Mat descriptors;
};

class FeatureProcessor {
public:
    FeatureProcessor();
    ~FeatureProcessor();

    // 应用掩码到帧数据
    void apply_boxes(DumpRes& dump_res, const std::vector<YOLOBbox>& yolo_results);

    // 执行特征提取和分配
    void process_features(DumpRes& dump_res,
                         const std::vector<YOLOBbox>& yolo_results,
                         std::vector<ObjectFeatures>& object_features_list,
                         SuperPoint& sp,
                         ORBExtractor& orb_extractor);

private:
    // 分配特征点到各个目标
    void assign_features_to_objects(const std::vector<cv::Point2f>& fused_keypoints,
                                   const cv::Mat& fused_descriptors,
                                   const std::vector<YOLOBbox>& yolo_results,
                                   std::vector<ObjectFeatures>& object_features_list);

    int frame_count = 0;  // 帧计数器
};

#endif // FEATURE_PROCESSOR_H