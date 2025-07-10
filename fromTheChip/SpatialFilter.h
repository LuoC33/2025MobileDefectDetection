
#pragma once

#include <vector>
#include <opencv2/core.hpp>
#include "ObjectDatabase.h"
#include "feature_processor.h"

// 网格索引配置
struct GridConfig {
    int grid_size = 50;       // 网格大小(像素)
    float search_radius = 80; // 搜索半径(像素)
    float velocity_factor = 1.2f; // 速度预测因子
    bool use_prediction = true;
};

// 运动预测结构体
struct MotionState {
    cv::Point2f last_position;
    cv::Point2f velocity;
    int last_frame_id;
};

class SpatialFilter {
public:
    /**
     * @brief 构造函数
     * @param width 图像宽度
     * @param height 图像高度
     * @param config 网格配置参数
     */
    SpatialFilter(int width, int height, ObjectDatabase& database, const GridConfig& config = GridConfig());
    
    /**
     * @brief 更新网格索引
     * @param targets 目标数据库中的目标列表
     */
    void updateGridIndex(const std::vector<TrackedObject>& targets);
    
    /**
     * @brief 查询候选目标
     * @param current_obj 当前帧的目标特征
     * @return 候选目标ID列表
     */
    std::vector<int> queryCandidates(const ObjectFeatures& current_feature);
    
    /**
     * @brief 获取网格配置
     * @return 网格配置引用
     */
    GridConfig& getConfig() { return config_; }

    // 获取目标中心点
    cv::Point2f getObjectCenter(const ObjectFeatures& obj) const;

    TrackedObject* getTargetByPosition(const cv::Point2f& position);

    /**
     * @brief 获取目标的运动状态
     * @param target_id 目标ID
     * @return 运动状态指针，如果不存在返回nullptr
     */
    const MotionState* getMotionState(int target_id) const;

private:
    // 计算网格坐标
    inline void positionToGrid(const cv::Point2f& pos, int& grid_x, int& grid_y) const;
    
private:
    int width_;                  // 图像宽度
    int height_;                 // 图像高度
    GridConfig config_;          // 网格配置
    
    // 网格索引数据结构
    int grid_cols_;              // 网格列数
    int grid_rows_;              // 网格行数
    std::vector<std::vector<std::vector<int>>> grid_; // 三维网格索引 [row][col][object_ids]
    ObjectDatabase& database_;
    std::unordered_map<int, cv::Point2f> position_cache_;
    std::unordered_map<int, MotionState> motion_cache_;
};