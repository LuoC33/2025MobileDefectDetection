/**
 * 表观特征过滤
 */
#pragma once

#include <vector>
#include <map>
#include <memory>
#include <opencv2/core.hpp>
#include <opencv2/features2d/features2d.hpp>
#include "ObjectDatabase.h"
#include "feature_processor.h"

// 视觉词袋模型配置
struct BoWConfig {
    int vocabulary_size = 500;    // 视觉词典大小
    int knn_candidates = 10;       // 返回的相似目标数量
    float min_similarity = 0.3f;   // 最小相似度阈值
    bool use_tfidf = true;         // TF-IDF加权
};

class AppearanceFilter {
public:
    /**
     * @brief 构造函数
     * @param vocabulary 预训练的视觉词典
     * @param config 配置参数
     */
    AppearanceFilter(const cv::Mat& vocabulary = cv::Mat(), 
                    const BoWConfig& config = BoWConfig());
    
    /**
     * @brief 初始化视觉词典
     * @param descriptors 训练描述子集合
     */
    void initializeVocabulary(const std::vector<cv::Mat>& descriptors);
    
    /**
     * @brief 更新倒排索引（在目标数据库更新后调用）
     * @param targets 目标数据库中的目标列表
     */
    void updateInvertedIndex(const std::vector<TrackedObject>& targets);
    
    /**
     * @brief 查询表观相似的目标
     * @param current_obj 当前帧的目标特征
     * @param candidate_ids 空间过滤后的候选目标ID（输入）
     * @param similar_targets 输出的相似目标列表（目标ID和相似度）
     * @return 是否找到相似目标
     */
    bool querySimilarTargets(const ObjectFeatures& current_obj,
                            const std::vector<int>& candidate_ids,
                            std::vector<std::pair<int, float>>& similar_targets);
    
    /**
     * @brief 计算一个目标的BoW向量
     * @param descriptors 目标的描述子矩阵
     * @return BoW向量（稀疏表示）
     */
    cv::Mat computeBowVector(const cv::Mat& descriptors);
    
    /**
     * @brief 获取配置
     * @return 配置引用
     */
    BoWConfig& getConfig() { return config_; }
    
    /**
     * @brief 获取视觉词典
     * @return 视觉词典矩阵
     */
    const cv::Mat& getVocabulary() const { return vocabulary_; }

    /**
     * @brief 从二进制文件加载视觉词典
     * @param filename 二进制文件路径
     * @return 加载成功返回 true
     */
    bool loadVocabularyFromBinary(const std::string& filename);

private:
    // 计算两个BoW向量的相似度（余弦相似度）
    float computeSimilarity(const cv::Mat& bow1, const cv::Mat& bow2);
    
    // 更新IDF权重（可选）
    void updateIDF();

private:
    cv::Mat vocabulary_; // 视觉词典 (vocabulary_size x 256, 类型为CV_32F)
    BoWConfig config_;
    
    // 倒排索引：word_id -> [目标ID列表]
    std::vector<std::vector<int>> inverted_index_;
    
    // 存储每个目标的BoW向量
    std::map<int, cv::Mat> target_bow_vectors_;
    
    // IDF权重（如果使用TF-IDF）
    cv::Mat idf_;
    
    // FLANN匹配器用于快速量化
    cv::Ptr<cv::flann::Index> flann_index_;
};