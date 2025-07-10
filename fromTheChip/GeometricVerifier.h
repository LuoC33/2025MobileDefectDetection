/**
 * 几何一致性验证
 */
#pragma once

#include <vector>
#include <map>
#include <opencv2/core.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include "ObjectDatabase.h"
#include "feature_processor.h"

// 几何验证配置
struct GeometricConfig {
    float min_inlier_ratio = 0.6f;      // 最小内点比例
    int min_inliers = 10;               // 最小内点数量
    float ransac_threshold = 3.0f;      // RANSAC阈值(像素)
    int ransac_max_iters = 30;         // RANSAC最大迭代次数
    float min_match_ratio = 0.5f;       // 最小匹配比例
    int max_candidates = 5;             // 最大验证候选数
    bool use_homography = true;         // 使用单应性矩阵(true)或基础矩阵(false)
    int max_features = 100; 
};

class GeometricVerifier {
public:
    /**
     * @brief 构造函数
     * @param config 配置参数
     */
    explicit GeometricVerifier(const GeometricConfig& config = GeometricConfig());
    
    /**
     * @brief 初始化特征匹配器
     */
    void initializeMatcher();
    
    /**
     * @brief 执行几何验证
     * @param current_obj 当前帧的目标特征
     * @param candidates 候选目标列表（来自表观过滤层）
     * @param verified_targets 通过验证的目标ID和内点数量
     * @return 是否找到通过验证的目标
     */
    bool verify(const ObjectFeatures& current_obj,
               const std::vector<std::pair<int, float>>& candidates,
               std::vector<std::pair<int, int>>& verified_targets);
    
    /**
     * @brief 获取配置
     * @return 配置引用
     */
    GeometricConfig& getConfig() { return config_; }

private:
    // 执行特征匹配
    void matchFeatures(const cv::Mat& desc1, const cv::Mat& desc2,
                      std::vector<cv::DMatch>& matches);
    
    // 执行几何验证
    bool geometricVerification(const std::vector<cv::Point2f>& pts1,
                              const std::vector<cv::Point2f>& pts2,
                              const std::vector<cv::DMatch>& matches,
                              std::vector<cv::DMatch>& inlier_matches);

private:
    GeometricConfig config_;
    cv::Ptr<cv::DescriptorMatcher> matcher_; // 特征匹配器
    std::map<int, std::vector<cv::Point2f>> normalized_points_cache_; // 归一化点缓存
};

// struct GeometricConfig {
//     float min_inlier_ratio = 0.6f;      // 内点比例阈值
//     int min_inliers = 10;               // 最小内点数
//     float ransac_threshold = 3.0f;      // RANSAC阈值(像素)
//     int ransac_max_iters = 100;         // RANSAC最大迭代次数
//     float min_match_ratio = 0.7f;       // 最小匹配比例
//     int max_candidates = 5;             // 最大验证候选数
//     bool use_homography = true;         // 验证方法
// };