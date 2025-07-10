/**
 * 目标数据库
 */
#pragma once

#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <deque>
#include <opencv2/core.hpp>
#include "ObjectDatabase.h"
#include "feature_processor.h"

// 跟踪目标结构体
struct TrackedObject {
    int id;                             // 目标唯一ID
    ObjectFeatures current_feature;     // 当前特征
    cv::Point2f predicted_position;
    cv::Point2f last_position;          // 上次出现的位置
    cv::Point2f velocity;               // 速度向量 (像素/帧)
    int last_frame_id;                  // 最后出现的帧ID
    float confidence;                   // 置信度 [0,1]
    int appearance_count;               // 出现次数
    int miss_count;                     // 连续丢失次数
    
    // 历史特征队列
    std::deque<ObjectFeatures> feature_history;
    
    TrackedObject() 
        : id(-1), last_frame_id(-1), confidence(1.0f), 
          appearance_count(0), miss_count(0) {}
    
    TrackedObject(int obj_id, const ObjectFeatures& feature, int frame_id)
        : id(obj_id), current_feature(feature), last_frame_id(frame_id),
          confidence(1.0f), appearance_count(1), miss_count(0) {
        // 初始化位置
        last_position = getObjectCenter();
        // 保存初始特征
        addFeatureHistory(feature);
        predicted_position = getObjectCenter();
        // 初始速度为零
        velocity = cv::Point2f(0, 0);
    }
    
    // 获取目标中心点
    cv::Point2f getObjectCenter() const {
        const cv::Rect& box = current_feature.bbox.box;
        return cv::Point2f(box.x + box.width/2.0f, box.y + box.height/2.0f);
    }
    
    // 添加特征到历史记录
    void addFeatureHistory(const ObjectFeatures& feature, int max_history = 3) {
        feature_history.push_front(feature);
        if (feature_history.size() > max_history) {
            feature_history.pop_back();
        }
    }
    
    // 更新目标状态
    void update(const ObjectFeatures& new_feature, int frame_id){
        if (last_frame_id < 0) {
            last_position = getObjectCenter();
        }

        // 保存旧位置
        cv::Point2f old_position = last_position;
        
        // 更新特征和位置
        current_feature = new_feature;
        last_position = getObjectCenter();
        last_frame_id = frame_id;
        
        // 更新速度
        if (last_frame_id >= 0 && frame_id > last_frame_id) {
            int frame_gap = frame_id - last_frame_id;
            if (frame_gap > 0) {
                cv::Point2f displacement = last_position - old_position;
                cv::Point2f new_velocity = displacement / static_cast<float>(frame_gap);
                
                // 使用指数平滑更新速度
                velocity = velocity * 0.3f + new_velocity * 0.7f;
            }
        }
        
        // 更新特征历史
        addFeatureHistory(new_feature);
        
        // 更新统计信息
        appearance_count++;
        miss_count = 0; // 重置丢失计数
        confidence = std::min(1.0f, confidence * 1.05f); // 增加置信度

        predicted_position = last_position + velocity;// 计算预测位置
        last_frame_id = frame_id;
    }
    
    // 更新丢失状态
    void updateMiss() {
        miss_count++;
        confidence *= 0.9f; // 每次丢失降低置信度
    }
};

// 目标数据库类
class ObjectDatabase {
public:
    // 获取数据库单例
    static ObjectDatabase& getInstance();
    
    // 禁止拷贝和赋值
    ObjectDatabase(const ObjectDatabase&) = delete;
    void operator=(const ObjectDatabase&) = delete;
    
    /**
     * @brief 添加新目标
     * @param feature 目标特征
     * @param frame_id 当前帧ID
     * @return 新目标的ID
     */
    int addNewTarget(const ObjectFeatures& feature, int frame_id);
    
    /**
     * @brief 更新目标
     * @param id 目标ID
     * @param feature 新特征
     * @param frame_id 当前帧ID
     * @return 是否成功更新
     */
    bool updateTarget(int id, const ObjectFeatures& feature, int frame_id);
    
    /**
     * @brief 获取目标
     * @param id 目标ID
     * @return 目标指针，不存在则返回nullptr
     */
    TrackedObject* getTarget(int id);
    
    /**
     * @brief 删除目标
     * @param id 目标ID
     */
    void removeTarget(int id);
    
    /**
     * @brief 获取所有活动目标
     * @param max_miss_count 最大允许丢失帧数
     * @return 目标列表
     */
    std::vector<TrackedObject> getAllActiveTargets(int max_miss_count = 5) const;
    
    /**
     * @brief 清除所有目标
     */
    void clear();
    
    /**
     * @brief 清理旧目标
     * @param max_miss_frames 最大允许丢失帧数
     * @param min_confidence 最小置信度
     */
    void cleanupOldTargets(int max_miss_frames = 10, float min_confidence = 0.3f);
    
    /**
     * @brief 获取目标数量
     * @return 目标数量
     */
    size_t size() const { return objects_.size(); }

    TrackedObject* getTargetByPosition(const cv::Point2f& position, float radius = 50.0f);

private:
    // 私有构造函数
    ObjectDatabase() : next_id_(1) {}
    
     // 目标存储
    std::atomic<int> next_id_;                    
    mutable std::mutex mutex_;             // 线程安全递归锁
    std::unordered_map<int, TrackedObject> objects_;
};