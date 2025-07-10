#include "SpatialFilter.h"
#include <cmath>
#include <algorithm>
#include <unordered_set>

SpatialFilter::SpatialFilter(int width, int height, ObjectDatabase& database, const GridConfig& config)
    : width_(width), height_(height),database_(database), config_(config) {
    
    // 计算网格行列数
    grid_cols_ = (width_ + config_.grid_size - 1) / config_.grid_size;
    grid_rows_ = (height_ + config_.grid_size - 1) / config_.grid_size;
    
    // 初始化网格结构
    grid_.resize(grid_rows_);
    for (auto& row : grid_) {
        row.resize(grid_cols_);
    }
}

void SpatialFilter::updateGridIndex(const std::vector<TrackedObject>& targets) {
    for (auto& row : grid_) {
        for (auto& col : row) {
            col.clear();
        }
    }
    
    // 将目标插入网格并更新运动状态
    for (const auto& target : targets) {
        int grid_x, grid_y;
        cv::Point2f predicted_position = target.predicted_position;
        positionToGrid(predicted_position, grid_x, grid_y);
        
        if (grid_y >= 0 && grid_y < grid_rows_ && grid_x >= 0 && grid_x < grid_cols_) {
            grid_[grid_y][grid_x].push_back(target.id);
            
            // 更新或创建独立的运动状态
            if (motion_cache_.find(target.id) == motion_cache_.end()) {
                motion_cache_[target.id] = MotionState{
                    target.last_position,
                    cv::Point2f(0, 0),
                    target.last_frame_id
                };
            } else {
                // 更新速度估计（使用指数平滑）
                MotionState& state = motion_cache_[target.id];
                int frame_gap = target.last_frame_id - state.last_frame_id;
                
                if (frame_gap > 0) {
                    cv::Point2f displacement = target.last_position - state.last_position;
                    cv::Point2f new_velocity = displacement / static_cast<float>(frame_gap);
                    
                    // 平滑更新速度
                    state.velocity = state.velocity * 0.3f + new_velocity * 0.7f;
                }
                
                // 更新位置和帧ID
                state.last_position = target.last_position;
                state.last_frame_id = target.last_frame_id;
            }
        }
    }
}

std::vector<int> SpatialFilter::queryCandidates(const ObjectFeatures& current_feature) {
    if (current_feature.keypoints.empty()) {
        return {}; // 无特征目标不处理
    }

    // 获取目标中心点
    cv::Point2f center = getObjectCenter(current_feature);
    
    float dynamic_radius = config_.search_radius;
    // 使用独立缓存的运动状态
    if (config_.use_prediction && !motion_cache_.empty()) {
        float max_speed = 0.0f;
        
        // 计算场景中最大速度
        for (const auto& [id, state] : motion_cache_) {
            float speed = cv::norm(state.velocity);
            if (speed > max_speed) max_speed = speed;
        }
        
        // 动态调整搜索半径：基础值 + 最大速度扩展
        if (max_speed > 0.1f) { // 忽略微小速度
            dynamic_radius = config_.search_radius + max_speed * config_.velocity_factor;
        }
    }
    
    // 确定搜索区域
    int min_grid_x, min_grid_y, max_grid_x, max_grid_y;
    positionToGrid(center - cv::Point2f(dynamic_radius, dynamic_radius), min_grid_x, min_grid_y);
    positionToGrid(center + cv::Point2f(dynamic_radius, dynamic_radius), max_grid_x, max_grid_y);
    
    // 限制在网格范围内
    min_grid_x = std::max(0, min_grid_x);
    min_grid_y = std::max(0, min_grid_y);
    max_grid_x = std::min(grid_cols_ - 1, max_grid_x);
    max_grid_y = std::min(grid_rows_ - 1, max_grid_y);
    
    std::unordered_set<int> candidate_set;

    // 遍历搜索区域内的网格
    for (int y = min_grid_y; y <= max_grid_y; y++) {
        for (int x = min_grid_x; x <= max_grid_x; x++) {
            for (int id : grid_[y][x]) {
                candidate_set.insert(id);
            }
        }
    }
    
    return std::vector<int>(candidate_set.begin(), candidate_set.end());
}

// 辅助函数：将位置转换为网格坐标
void SpatialFilter::positionToGrid(const cv::Point2f& pos, int& grid_x, int& grid_y) const {
    grid_x = static_cast<int>(pos.x) / config_.grid_size;
    grid_y = static_cast<int>(pos.y) / config_.grid_size;
}

// 辅助函数：获取目标中心点
cv::Point2f SpatialFilter::getObjectCenter(const ObjectFeatures& obj) const {
    return cv::Point2f(
        obj.bbox.box.x + obj.bbox.box.width / 2.0f,
        obj.bbox.box.y + obj.bbox.box.height / 2.0f
    );
}

TrackedObject* SpatialFilter::getTargetByPosition(const cv::Point2f& position) {
    printf("1\n");
    // 获取所有活动目标
    auto active_targets = database_.getAllActiveTargets();
    printf("1\n");
    
    // 寻找距离最近的目标
    TrackedObject* nearest_target = nullptr;
    float min_distance = std::numeric_limits<float>::max();
    printf("11\n");
    
    for (auto& target : active_targets) {
        float dist = cv::norm(target.getObjectCenter() - position);
        if (dist < config_.search_radius && dist < min_distance) {
            min_distance = dist;
            nearest_target = &const_cast<TrackedObject&>(target);
        }
    }
    printf("111\n");
    
    return nearest_target;
}

const MotionState* SpatialFilter::getMotionState(int target_id) const {
    auto it = motion_cache_.find(target_id);
    if (it != motion_cache_.end()) {
        return &(it->second);
    }
    return nullptr;
}