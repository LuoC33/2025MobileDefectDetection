#include "TemporalFilter.h"
#include <algorithm>
#include <cmath>
#include "ObjectDatabase.h"

using namespace cv;

TemporalFilter::TemporalFilter(const TemporalConfig& config,SpatialFilter& spatial)
    : config_(config),spatial_(spatial) {}

bool TemporalFilter::verify(const ObjectFeatures& current_obj,
                          const std::vector<int>& candidate_ids,
                          int current_frame_id,
                          int& matched_id) {
    matched_id = -1;
    if (candidate_ids.empty()) {
        return false;
    }   
    // if (target->appearance_count < config_.min_track_length) {
    //     continue; // 跳过历史不足的目标
    // }
    
    // 获取当前目标中心
    cv::Point2f current_center = getObjectCenter(current_obj);
    
    float best_score = 0.0f;
    int best_target_id = -1;
    
    ObjectDatabase& db = ObjectDatabase::getInstance();
    
    for (int target_id : candidate_ids) {
        const TrackedObject* target = db.getTarget(target_id);
        if (!target) continue;
        
        // 计算运动一致性得分
        float motion_score = computeMotionConsistency(target_id, current_center);
        
        // 计算时间连续性得分
        float temporal_score = computeTemporalContinuity(target_id, current_frame_id);
        
        // 计算外观稳定性得分
        float appearance_score = computeAppearanceStability(target_id, current_obj);
        
        // 综合得分 = 运动得分 × 时间得分 × 外观得分
        float total_score = motion_score * temporal_score * appearance_score;
        
        printf("Target %d: motion=%.2f, temporal=%.2f, appear=%.2f, total=%.2f, frames=%d\n",
            target_id, motion_score, temporal_score, appearance_score, total_score, 
            target->appearance_count);
        
        if (total_score > best_score && total_score >= config_.min_continuity_score) {
            best_score = total_score;
            best_target_id = target_id;
        }
    }
    if (best_target_id != -1) {
        matched_id = best_target_id;
        return true;
    }

    return false;
}

float TemporalFilter::computeMotionConsistency(int target_id, const cv::Point2f& current_pos) const {
    const TrackedObject* target = ObjectDatabase::getInstance().getTarget(target_id);
    if (!target) return 0.0f;
    
    // 从空间过滤层获取运动状态
    const MotionState* state = spatial_.getMotionState(target_id);
    if (!state) return 0.0f;
    
    // 使用空间过滤层提供的运动状态
    float speed = cv::norm(state->velocity);
    
    if (speed < 0.1f) {
        float dist = cv::norm(current_pos - state->last_position);
        return (dist < 5.0f) ? 1.0f : (1.0f - dist/config_.max_speed_variation);
    }
    
    cv::Point2f predicted_pos = state->last_position + state->velocity;
    float dist = cv::norm(current_pos - predicted_pos);
    float normalized_dist = dist / speed;
    
    return 1.0f - std::min(normalized_dist / config_.max_speed_variation, 1.0f);
}

float TemporalFilter::computeTemporalContinuity(int target_id, int current_frame_id) const {
    const TrackedObject* target = ObjectDatabase::getInstance().getTarget(target_id);
    if (!target) return 0.0f;
    
    // 计算帧间隔
    int frame_gap = current_frame_id - target->last_frame_id;
    
    // 如果帧间隔为1，则连续出现
    if (frame_gap == 1) {
        return 1.0f;
    }
    
    // 如果帧间隔超过最大允许间隔，则得分为0
    if (frame_gap > config_.max_frame_gap) {
        return 0.0f;
    }
    
    // 衰减：每多一帧，得分乘以衰减因子
    return std::pow(config_.decay_factor, frame_gap - 1);
}

float TemporalFilter::computeAppearanceStability(int target_id, const ObjectFeatures& current_obj) const {
    const TrackedObject* target = ObjectDatabase::getInstance().getTarget(target_id);
    if (!target) return 0.0f;
    
    // 如果目标没有历史特征，返回1
    if (target->feature_history.empty()) {
        return 1.0f;
    }
    
    // 计算当前特征与历史特征的相似度
    float total_similarity = 0.0f;
    int count = 0;
    
    for (const auto& historical_feature : target->feature_history) {
        int current_size = current_obj.keypoints.size();
        int hist_size = historical_feature.keypoints.size();
        
        // 处理除零情况
        if (current_size == 0 && hist_size == 0) {
            total_similarity += 1.0f; // 都无特征视为相同
        } else if (current_size == 0 || hist_size == 0) {
            total_similarity += 0.0f; // 一个有特征一个无特征视为不同
        } else {
            float size_diff = std::abs(current_size - hist_size);
            float size_sum = current_size + hist_size;
            float size_sim = 1.0f - size_diff / size_sum;
            total_similarity += size_sim;
        }
        count++;
    }
    
    return (count > 0) ? (total_similarity / count) : 0.0f;
}

cv::Point2f TemporalFilter::getObjectCenter(const ObjectFeatures& obj) const {
    return cv::Point2f(
        obj.bbox.box.x + obj.bbox.box.width / 2.0f,
        obj.bbox.box.y + obj.bbox.box.height / 2.0f
    );
}

void TemporalFilter::updateTargetState(int target_id, const cv::Point2f& current_pos, int current_frame_id) {
    TrackedObject* target = ObjectDatabase::getInstance().getTarget(target_id);
    if (!target) return;
    
    // 更新速度和位置
    if (target->last_frame_id >= 0) { // 不是第一帧
        int frame_gap = current_frame_id - target->last_frame_id;
        if (frame_gap > 0) {
            // 计算速度 = (当前位置 - 上次位置) / 帧间隔
            cv::Point2f velocity = (current_pos - target->last_position) / static_cast<float>(frame_gap);
            
            // 平滑更新速度（指数平滑）
            target->velocity = target->velocity * 0.7f + velocity * 0.3f;
        }
    }
    
    // 更新位置和帧ID
    target->last_position = current_pos;
    target->last_frame_id = current_frame_id;
    
    // 更新置信度（可选）
    target->confidence = std::min(1.0f, target->confidence * 1.05f); // 匹配成功增加置信度
}