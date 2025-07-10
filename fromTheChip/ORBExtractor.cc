#include "ORBExtractor.h"
#include <chrono>

ORBExtractor::ORBExtractor(int max_features, float scale_factor,int n_levels,int fast_threshold,int debug_mode) :
    max_features_(max_features),
    scale_factor_(scale_factor),
    n_levels_(n_levels),
    fast_threshold_(fast_threshold),
    debug_mode_(debug_mode)
{
    // 初始化ORB检测器
    orb_detector_ = cv::ORB::create(
        max_features_,
        scale_factor_,
        n_levels_,
        15,  // edgeThreshold
        0,   // firstLevel
        2,   // WTA_K
        cv::ORB::FAST_SCORE,
        15,  // patchSize
        fast_threshold_);
}

void ORBExtractor::extract(const cv::Mat& image, std::vector<cv::Point2f>& keypoints,cv::Mat& descriptors)
{
    auto start = std::chrono::high_resolution_clock::now();
    
    // 清空输出
    keypoints.clear();
    descriptors.release();
    
    //  检测ORB特征点
    orb_detector_->detectAndCompute(image, cv::noArray(), orb_keypoints_, orb_descriptors_);
    
    //  计算描述子
    if (!orb_keypoints_.empty() && !orb_descriptors_.empty()) 
    {
        // 转换为SuperPoint兼容格式
        // keypoints.reserve(orb_keypoints_.size());
        // for (const auto& kp : orb_keypoints_) {
        //     keypoints.emplace_back(kp.pt);
        // }

        keypoints.resize(orb_keypoints_.size());
        #pragma omp parallel for
        for (size_t i = 0; i < orb_keypoints_.size(); ++i) {
            keypoints[i] = orb_keypoints_[i].pt;  // 直接赋值
        }        
        
        // 转换描述子格式为CV_8S (与SuperPoint一致)
        if (orb_descriptors_.type() != CV_8S) {
            orb_descriptors_.convertTo(descriptors, CV_8S);
        } else {
            //descriptors = orb_descriptors_.clone();
            descriptors = orb_descriptors_;
        }
        
        // L2归一化(与SuperPoint一致)
        // for (int i = 0; i < descriptors.rows; ++i) {
        //     cv::normalize(descriptors.row(i), descriptors.row(i));
        // }
        //cv::normalize(descriptors, descriptors, 1.0, 0, cv::NORM_L2);

    }
    
    if (debug_mode_ > 0) {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        printf("[ORB] Extracted %zu keypoints in %lld ms\n", 
               keypoints.size(), duration.count());
    }
}

void ORBExtractor::fuse_features(const std::vector<cv::Point2f>& sp_keypoints,
                               const cv::Mat& sp_descriptors,
                               const std::vector<cv::Point2f>& orb_keypoints,
                               const cv::Mat& orb_descriptors,
                               std::vector<cv::Point2f>& fused_keypoints,
                               cv::Mat& fused_descriptors,
                               float min_distance)
{
    // 清空输出
    fused_keypoints.clear();
    fused_descriptors.release();
    std::cerr << "111\n" << std::endl;
    // 合并特征点
    fused_keypoints = sp_keypoints;
    sp_descriptors.copyTo(fused_descriptors);
    std::cerr << "111\n" << std::endl;
    // 添加ORB特征点(过滤掉靠近SuperPoint点的特征)
    if (!orb_keypoints.empty() && !orb_descriptors.empty()) {
        for (size_t i = 0; i < orb_keypoints.size(); ++i) {
            bool too_close = false;
            std::cerr << "111\n" << std::endl;
            // 检查与所有SuperPoint点的距离
            for (const auto& sp_kp : sp_keypoints) {
                if (cv::norm(sp_kp - orb_keypoints[i]) < min_distance) {
                    too_close = true;
                    break;
                }
            }
            std::cerr << "111\n" << std::endl;
            // 只保留距离足够远的ORB特征
            if (!too_close) {
                fused_keypoints.push_back(orb_keypoints[i]);
                fused_descriptors.push_back(orb_descriptors.row(i));
            }
            std::cerr << "111\n" << std::endl;
        }
    }
}