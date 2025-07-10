#include "GeometricVerifier.h"
#include <algorithm>
#include <cmath>

using namespace cv;

GeometricVerifier::GeometricVerifier(const GeometricConfig& config)
    : config_(config) {
    initializeMatcher();
}

void GeometricVerifier::initializeMatcher() {
    // 使用FLANN匹配器，适合二进制描述子
    matcher_ = cv::DescriptorMatcher::create(cv::DescriptorMatcher::FLANNBASED);
}

bool GeometricVerifier::verify(const ObjectFeatures& current_obj,
                             const std::vector<std::pair<int, float>>& candidates,
                             std::vector<std::pair<int, int>>& verified_targets) {
    verified_targets.clear();
    
    // 获取当前目标特征
    const auto& current_kpts = current_obj.keypoints;
    const auto& current_desc = current_obj.descriptors;

    // if (current_kpts.size() > config_.max_features) {
    //     current_kpts.resize(config_.max_features);
    //     current_desc = current_desc.rowRange(0, config_.max_features);
    // }
    // 没有足够特征点则跳过
    if (current_kpts.size() < 30) {
        return false;
    }
    
    // 按相似度排序候选目标
    std::vector<std::pair<int, float>> sorted_candidates = candidates;
    std::sort(sorted_candidates.begin(), sorted_candidates.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // 限制候选数量
    if (sorted_candidates.size() > config_.max_candidates) {
        sorted_candidates.resize(config_.max_candidates);
    }
    
    // 对每个候选目标进行验证
    for (const auto& candidate : sorted_candidates) {
        int target_id = candidate.first;
        
        // 从数据库获取候选目标特征
        const auto& target_obj = ObjectDatabase::getInstance().getTarget(target_id);
        const auto& target_kpts = target_obj->current_feature.keypoints;
        const auto& target_desc = target_obj->current_feature.descriptors;
        
        // 没有足够特征点则跳过
        if (target_kpts.size() < 30) {
            continue;
        }
        
        // 执行特征匹配
        std::vector<cv::DMatch> matches;
        matchFeatures(current_desc, target_desc, matches);
        
        // 检查匹配数量
        if (matches.size() < 4) {
            continue;
        }
        
        // 计算匹配比例
        float match_ratio = static_cast<float>(matches.size()) / 
                           std::min(current_kpts.size(), target_kpts.size());
        
        if (match_ratio < config_.min_match_ratio) {
            continue;
        }
        
        // 准备匹配点对
        std::vector<Point2f> pts1, pts2;
        pts1.reserve(matches.size());
        pts2.reserve(matches.size());
        
        for (const auto& m : matches) {
            pts1.push_back(current_kpts[m.queryIdx]);
            pts2.push_back(target_kpts[m.trainIdx]);
        }
        
        // 执行几何验证
        std::vector<cv::DMatch> inlier_matches;
        bool is_valid = geometricVerification(pts1, pts2, matches, inlier_matches);
        
        if (is_valid) {
            verified_targets.emplace_back(target_id, static_cast<int>(inlier_matches.size()));
        }
    }
    
    // 按内点数量排序
    std::sort(verified_targets.begin(), verified_targets.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    return !verified_targets.empty();
}

void GeometricVerifier::matchFeatures(const cv::Mat& desc1, const cv::Mat& desc2,
                                    std::vector<cv::DMatch>& matches) {
    // 使用knnMatch获取前2个最佳匹配
    std::vector<std::vector<cv::DMatch>> knn_matches;
    matcher_->knnMatch(desc1, desc2, knn_matches, 2);
    
    // 应用比率测试过滤误匹配
    matches.clear();
    for (size_t i = 0; i < knn_matches.size(); i++) {
        if (knn_matches[i].size() < 2) continue;
        
        const cv::DMatch& best_match = knn_matches[i][0];
        const cv::DMatch& second_match = knn_matches[i][1];
        
        // Lowe's 比率测试
        if (best_match.distance < 0.8 * second_match.distance) {
            matches.push_back(best_match);
        }
    }
}

// void GeometricVerifier::matchFeatures(const cv::Mat& desc1, const cv::Mat& desc2,
//                                     std::vector<cv::DMatch>& matches) {
//     // 使用BFMatcher with Hamming距离
//     cv::BFMatcher matcher(cv::NORM_HAMMING);
//     std::vector<std::vector<cv::DMatch>> knn_matches;
//     matcher.knnMatch(desc1, desc2, knn_matches, 2);
    
//     // 应用比率测试
//     matches.clear();
//     for (size_t i = 0; i < knn_matches.size(); i++) {
//         if (knn_matches[i].size() < 2) continue;
//         if (knn_matches[i][0].distance < 0.7 * knn_matches[i][1].distance) {
//             matches.push_back(knn_matches[i][0]);
//         }
//     }
// }

bool GeometricVerifier::geometricVerification(const std::vector<cv::Point2f>& pts1,
                                            const std::vector<cv::Point2f>& pts2,
                                            const std::vector<cv::DMatch>& matches,
                                            std::vector<cv::DMatch>& inlier_matches) {
    if (pts1.size() < 4 || pts2.size() < 4) {
        return false;
    }
    
    // 使用RANSAC进行几何验证
    std::vector<uchar> inliers;
    
    if (config_.use_homography) {
        // 单应性矩阵验证
        cv::Mat H = cv::findHomography(
            pts1, pts2,
            cv::RANSAC,
            config_.ransac_threshold,
            inliers,
            config_.ransac_max_iters
        );
    } else {
        // 基础矩阵验证
        cv::Mat camera_matrix = cv::Mat::eye(3, 3, CV_64F);
        cv::Mat F = cv::findFundamentalMat(
            pts1, pts2,
            cv::FM_RANSAC,
            config_.ransac_threshold,
            0.99,
            inliers
        );
    }
    
    // 统计内点
    int inlier_count = 0;
    for (size_t i = 0; i < inliers.size(); i++) {
        if (inliers[i]) {
            inlier_count++;
            inlier_matches.push_back(matches[i]);
        }
    }
    
    // 计算内点比例
    float inlier_ratio = static_cast<float>(inlier_count) / matches.size();
    
    // 验证条件
    bool is_valid = (inlier_count >= config_.min_inliers) && 
                   (inlier_ratio >= config_.min_inlier_ratio);
    
    return is_valid;
}