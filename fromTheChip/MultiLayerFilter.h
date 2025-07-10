/**
 * 过滤层统一接口
 */
#pragma once

#include "SpatialFilter.h"
#include "AppearanceFilter.h"
#include "GeometricVerifier.h"
#include "TemporalFilter.h"
#include "ObjectDatabase.h"
#include "feature_processor.h"

struct TemporalConfig;
struct GridConfig;
struct BoWConfig;
struct GeometricConfig;

// 多层过滤配置
struct FilterConfig {
    GridConfig spatial;
    BoWConfig appearance;
    GeometricConfig geometric;
    TemporalConfig temporal;
};

class MultiLayerFilter {
public:
    /**
     * @brief 构造函数
     * @param width 图像宽度
     * @param height 图像高度
     * @param config 配置参数
     */
    MultiLayerFilter(int width, int height, const FilterConfig& config = FilterConfig());
    
    /**
     * @brief 处理新一帧的目标
     * @param features_list 目标特征列表
     * @param frame_id 当前帧ID
     * @return 跟踪目标ID列表
     */
    std::vector<int> processFrame(const std::vector<ObjectFeatures>& features_list, int frame_id);
    
    /**
     * @brief 获取目标数据库引用
     * @return 目标数据库引用
     */
    ObjectDatabase& getDatabase() { return database_; }
    
    /**
     * @brief 清理旧目标
     */
    void cleanupOldTargets() {
        database_.cleanupOldTargets(10, 0.2f);
    }

    /**
     * @brief 获取新目标列表
     */
    const std::vector<ObjectFeatures>& getNewObjects() const { return new_objects_; }

    SpatialFilter& getSpatialFilter() { return spatial_filter_; }
    
private:
    // 处理单个目标
    int processObject(const ObjectFeatures& feature, int frame_id);
    
    // 创建新目标
    int createNewTarget(const ObjectFeatures& feature, int frame_id);
    
    // 更新匹配目标状态
    void updateMatchedTarget(int target_id, 
                            const ObjectFeatures& feature, 
                            const cv::Point2f& center,
                            int frame_id);

private:
    SpatialFilter spatial_filter_;
    TemporalFilter temporal_filter_;
    AppearanceFilter appearance_filter_;
    GeometricVerifier geometric_verifier_;
    ObjectDatabase& database_;
    
    FilterConfig config_;
    std::vector<ObjectFeatures> new_objects_;
};