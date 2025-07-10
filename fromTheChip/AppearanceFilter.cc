#include "AppearanceFilter.h"
#include <cmath>
#include <algorithm>
#include <opencv2/flann/flann.hpp>
#include <fstream>
#include <iostream>

AppearanceFilter::AppearanceFilter(const cv::Mat& vocabulary, const BoWConfig& config)
    : config_(config) {
    
    if (!vocabulary.empty()) {
        vocabulary_ = vocabulary;
        // 创建FLANN索引用于快速量化
        flann_index_ = cv::makePtr<cv::flann::Index>(
            vocabulary_, 
            cv::flann::KDTreeIndexParams(1)
        );
    }
    
    // 初始化倒排索引
    inverted_index_.resize(config_.vocabulary_size);
}

void AppearanceFilter::initializeVocabulary(const std::vector<cv::Mat>& descriptors) {
    if (descriptors.empty()) return;
    
    // 合并所有描述子
    cv::Mat merged_descriptors;
    for (const auto& desc : descriptors) {
        merged_descriptors.push_back(desc);
    }
    
    // 使用k-means聚类创建视觉词典
    cv::Mat labels, centers;
    int attempts = 3;
    cv::kmeans(
        merged_descriptors, 
        config_.vocabulary_size, 
        labels, 
        cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::MAX_ITER, 100, 0.01),
        attempts, 
        cv::KMEANS_PP_CENTERS, 
        centers
    );
    
    vocabulary_ = centers;
    
    // 创建FLANN索引
    flann_index_ = cv::makePtr<cv::flann::Index>(
        vocabulary_, 
        cv::flann::KDTreeIndexParams(1)
    );

    if (vocabulary_.empty()) {
        // 创建空索引或使用默认词典
        vocabulary_ = cv::Mat::zeros(config_.vocabulary_size, 256, CV_32F);
        flann_index_ = cv::makePtr<cv::flann::Index>(vocabulary_, cv::flann::KDTreeIndexParams(1));
    }
}

void AppearanceFilter::updateInvertedIndex(const std::vector<TrackedObject>& targets) {
    // 重置倒排索引
    for (auto& list : inverted_index_) {
        list.clear();
    }
    target_bow_vectors_.clear();
    
    // 重新计算每个目标的BoW向量并更新倒排索引
    for (const auto& target : targets) {
        // 计算BoW向量
        cv::Mat bow_vec = computeBowVector(target.current_feature.descriptors);
        target_bow_vectors_[target.id] = bow_vec;
        
        // 更新倒排索引：找到非零元素
        for (int i = 0; i < bow_vec.cols; i++) {
            float weight = bow_vec.at<float>(0, i);
            if (weight > 0) {
                inverted_index_[i].push_back(target.id);
            }
        }
    }
    
    // 更新IDF权重
    if (config_.use_tfidf) {
        updateIDF();
    }
}

bool AppearanceFilter::querySimilarTargets(const ObjectFeatures& current_obj,
                                         const std::vector<int>& candidate_ids,
                                         std::vector<std::pair<int, float>>& similar_targets) {
    similar_targets.clear();

    
    // 计算当前目标的BoW向量
    cv::Mat current_bow = computeBowVector(current_obj.descriptors);
    if (current_bow.empty()) {
        return false;
    }
    // 用于记录每个候选目标的相似度
    std::map<int, float> candidate_similarities;
    
    // 通过倒排索引收集共享单词的目标
    for (int word_id = 0; word_id < config_.vocabulary_size; word_id++) {
        float weight = current_bow.at<float>(0, word_id);
        if (weight > 0) {
            for (int target_id : inverted_index_[word_id]) {
                // 如果这个目标在空间过滤的候选列表中
                if (std::find(candidate_ids.begin(), candidate_ids.end(), target_id) != candidate_ids.end()) {
                    // 计算相似度（如果尚未计算）
                    if (candidate_similarities.find(target_id) == candidate_similarities.end()) {
                        float sim = computeSimilarity(current_bow, target_bow_vectors_[target_id]);
                        candidate_similarities[target_id] = sim;
                    }
                }
            }
        }
    }
    
    // 将候选目标按相似度排序
    for (const auto& pair : candidate_similarities) {
        if (pair.second >= config_.min_similarity) {
            similar_targets.push_back(pair);
        }
    }
    
    // 按相似度降序排序
    std::sort(similar_targets.begin(), similar_targets.end(),
              [](const std::pair<int, float>& a, const std::pair<int, float>& b) {
                  return a.second > b.second;
              });
    
    // 如果数量超过knn_candidates，则截断
    if (similar_targets.size() > config_.knn_candidates) {
        similar_targets.resize(config_.knn_candidates);
    }
    
    return !similar_targets.empty();
}

cv::Mat AppearanceFilter::computeBowVector(const cv::Mat& descriptors) {
    if (descriptors.empty() || !flann_index_ || vocabulary_.empty()) {
        // 返回均匀分布向量作为fallback
        return cv::Mat::ones(1, config_.vocabulary_size, CV_32F) / config_.vocabulary_size;
    }
    
    // 量化描述子到视觉单词
    cv::Mat indices(descriptors.rows, 1, CV_32S);
    cv::Mat dists(descriptors.rows, 1, CV_32F);
    flann_index_->knnSearch(descriptors, indices, dists, 1, cv::flann::SearchParams());

    // 创建词频向量
    cv::Mat bow_vector = cv::Mat::zeros(1, config_.vocabulary_size, CV_32F);
    for (int i = 0; i < descriptors.rows; i++) {
        int word_id = indices.at<int>(i, 0);
        bow_vector.at<float>(0, word_id) += 1.0f; // 词频加1
    }
    
    // 归一化词频（TF）
    cv::normalize(bow_vector, bow_vector, 1.0, 0, cv::NORM_L1);

    // 应用TF-IDF加权（如果启用）
    if (config_.use_tfidf && !idf_.empty()) {
        bow_vector = bow_vector.mul(idf_);
        // 重新归一化
        cv::normalize(bow_vector, bow_vector, 1.0, 0, cv::NORM_L1);
    }
    
    return bow_vector;
}

bool AppearanceFilter::loadVocabularyFromBinary(const std::string& filename) {
    // 打开二进制文件
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Failed to open vocabulary file: " << filename << std::endl;
        return false;
    }

    // 获取文件大小
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    // 验证文件大小 (500个单词 x 256维 x 4字节)
    const size_t expected_size = config_.vocabulary_size * 256 * sizeof(float);
    if (file_size != expected_size) {
        std::cerr << "Error: Invalid vocabulary file size. Expected: " 
                  << expected_size << " bytes, Got: " << file_size << " bytes" << std::endl;
        return false;
    }

    // 读取数据到临时缓冲区
    std::vector<float> buffer(config_.vocabulary_size * 256);
    file.read(reinterpret_cast<char*>(buffer.data()), file_size);
    file.close();

    // 创建 OpenCV 矩阵 (直接使用内存映射)
    vocabulary_ = cv::Mat(
        config_.vocabulary_size, 
        256, 
        CV_32F, 
        buffer.data()
    ).clone();

    // 重建 FLANN 索引
    flann_index_ = cv::makePtr<cv::flann::Index>(
        vocabulary_, 
        cv::flann::KDTreeIndexParams(1)
    );

    std::cout << "Loaded vocabulary with " << vocabulary_.rows 
              << " words from " << filename << std::endl;
    return true;
}

float AppearanceFilter::computeSimilarity(const cv::Mat& bow1, const cv::Mat& bow2) {
    // 计算余弦相似度
    return bow1.dot(bow2) / (cv::norm(bow1) * cv::norm(bow2));
}

void AppearanceFilter::updateIDF() {
    if (target_bow_vectors_.empty()) {
        return;
    }
    // 计算每个单词的文档频率
    cv::Mat df = cv::Mat::zeros(1, config_.vocabulary_size, CV_32F);
    int total_targets = target_bow_vectors_.size();
    
    for (const auto& pair : target_bow_vectors_) {
        const cv::Mat& bow_vec = pair.second;
        for (int i = 0; i < config_.vocabulary_size; i++) {
            if (bow_vec.at<float>(0, i) > 0) {
                df.at<float>(0, i) += 1.0f;
            }
        }
    }
    
    idf_ = cv::Mat::zeros(1, config_.vocabulary_size, CV_32F);
    for (int i = 0; i < config_.vocabulary_size; i++) {
        float df_val = df.at<float>(0, i);
        if (df_val > 0) {
            idf_.at<float>(0, i) = std::log(total_targets / (df_val + 1));
        }
    }
}