/**
 * 时序连续性验证
 */
#pragma once

#include <vector>
#include <map>
#include <deque>
#include <cmath>
#include <opencv2/core.hpp>
#include "ObjectDatabase.h"
#include "feature_processor.h"
#include "SpatialFilter.h"

// 时序验证配置
struct TemporalConfig {
    int max_frame_gap = 5;             // 最大允许的帧间隔
    float min_confidence = 0.6f;        // 最小匹配置信度
    float decay_factor = 0.9f;          // 置信度衰减因子
    int min_track_length = 3;           // 最小轨迹长度
    float max_speed_variation = 0.5f;   // 最大速度变化比例
    float min_continuity_score = 0.4f;  // 最小连续性得分
};

class TemporalFilter {
public:
    /**
     * @brief 构造函数
     * @param config 配置参数
     */
    explicit TemporalFilter(const TemporalConfig& config,SpatialFilter &spatial) ;
    
    /**
     * @brief 验证时序连续性
     * @param current_obj 当前帧的目标特征
     * @param candidate_ids 候选目标ID列表（来自几何验证层）
     * @param current_frame_id 当前帧ID
     * @param matched_id 输出的匹配目标ID
     * @return 是否找到匹配目标
     */
    bool verify(const ObjectFeatures& current_obj,
                const std::vector<int>& candidate_ids,
                int current_frame_id,
                int& matched_id);
    
    /**
     * @brief 更新目标状态（在目标匹配成功后调用）
     * @param target_id 目标ID
     * @param current_pos 当前位置
     * @param current_frame_id 当前帧ID
     */
    void updateTargetState(int target_id, const cv::Point2f& current_pos, int current_frame_id);
    
    /**
     * @brief 获取配置
     * @return 配置引用
     */
    TemporalConfig& getConfig() { return config_; }

    // 获取目标中心点
    cv::Point2f getObjectCenter(const ObjectFeatures& obj) const;
    
private:
    // 计算运动一致性得分
    float computeMotionConsistency(int target_id, const cv::Point2f& current_pos) const;
    
    // 计算时间连续性得分
    float computeTemporalContinuity(int target_id, int current_frame_id) const;
    
    // 计算外观稳定性得分
    float computeAppearanceStability(int target_id, const ObjectFeatures& current_obj) const;
    


private:
    TemporalConfig config_;
    SpatialFilter& spatial_;
};


