#include "MultiLayerFilter.h"

MultiLayerFilter::MultiLayerFilter(int width, int height, const FilterConfig& config)
    : database_(ObjectDatabase::getInstance()),
      spatial_filter_(width, height, database_, config.spatial),
      temporal_filter_(config.temporal,spatial_filter_),
      appearance_filter_(cv::Mat(), config.appearance), 
      geometric_verifier_(config.geometric),
      config_(config) {
    if (!appearance_filter_.loadVocabularyFromBinary("vocab.bin")) {
        std::cerr << "Falling back to default vocabulary" << std::endl;    
    }
    config_.temporal.decay_factor = 0.95f; 
    // 降低最小连续性得分要求
    config_.temporal.min_continuity_score = 0.3f;
    // 调整运动一致性阈值
    config_.temporal.max_speed_variation = 0.8f;
}

std::vector<int> MultiLayerFilter::processFrame(const std::vector<ObjectFeatures>& features_list, int frame_id) {
    new_objects_.clear();

    std::vector<int> tracked_ids;

    // 更新空间索引
    auto active_targets = database_.getAllActiveTargets();
    spatial_filter_.updateGridIndex(active_targets);

    // 更新外观索引
    appearance_filter_.updateInvertedIndex(active_targets);

    // 处理每个目标
    for (const auto& feature : features_list) {
        int target_id = processObject(feature, frame_id);
        if (target_id != -1) {
            tracked_ids.push_back(target_id);
        }
    }
    
    // 更新数据库中的丢失目标
    for (auto& target : active_targets) {
        if (std::find(tracked_ids.begin(), tracked_ids.end(), target.id) == tracked_ids.end()) {
            if (TrackedObject* db_target = database_.getTarget(target.id)) {
                if (db_target->miss_count < 3) {
                    db_target->updateMiss();
                } else {
                    // 超过3帧未匹配，从活动目标移除
                    database_.removeTarget(target.id);
                }
            }
        }
    }
    
    return tracked_ids;
}

int MultiLayerFilter::processObject(const ObjectFeatures& feature, int frame_id) {
    cv::Point2f center = temporal_filter_.getObjectCenter(feature);

    printf("Ingress space filtering\n");
    // 空间过滤
    std::vector<int> spatial_candidates = spatial_filter_.queryCandidates(feature);
    
    if (spatial_candidates.empty()) {
        return createNewTarget(feature, frame_id);
    }

    printf("Ingress time filtering\n");

    // 时序过滤
    int matched_id = -1;
    if (temporal_filter_.verify(feature, spatial_candidates, frame_id, matched_id)) {
        printf("Matched by temporal filter: target %d\n", matched_id);
        // 更新匹配目标状态
        updateMatchedTarget(matched_id, feature, center, frame_id);
        return matched_id;
    }

    printf("Ingress appearance filtering\n");

    // 表观过滤
    std::vector<std::pair<int, float>> appearance_candidates;
    if (!appearance_filter_.querySimilarTargets(feature, spatial_candidates, appearance_candidates)) {
        return createNewTarget(feature, frame_id);
    }

    printf("Ingress geometry filtering\n");

    // 几何验证
    std::vector<std::pair<int, int>> geometric_candidates;
    if (!geometric_verifier_.verify(feature, appearance_candidates, geometric_candidates)) {
        return createNewTarget(feature, frame_id);
    }
    
    int best_geometric_id = geometric_candidates[0].first;
    printf("Matched by geometric verifier: target %d\n", best_geometric_id);
    
    // 更新匹配目标状态
    updateMatchedTarget(best_geometric_id, feature, center, frame_id);
    return best_geometric_id;
}

int MultiLayerFilter::createNewTarget(const ObjectFeatures& feature, int frame_id) {
    int new_id = database_.addNewTarget(feature, frame_id);
    new_objects_.push_back(feature);
    printf("New target created! ID: %d\n", new_id);
    return new_id;
}

// 更新匹配目标状态
void MultiLayerFilter::updateMatchedTarget(int target_id, 
                                         const ObjectFeatures& feature, 
                                         const cv::Point2f& center,
                                         int frame_id) {
    // 更新目标特征
    database_.updateTarget(target_id, feature, frame_id);
    
    // 更新目标位置状态
    temporal_filter_.updateTargetState(target_id, center, frame_id);
}