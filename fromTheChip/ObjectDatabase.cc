#include "ObjectDatabase.h"
#include <algorithm>

// 获取单例实例
ObjectDatabase& ObjectDatabase::getInstance() {
    static ObjectDatabase instance;
    return instance;
}

int ObjectDatabase::addNewTarget(const ObjectFeatures& feature, int frame_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    int new_id = next_id_++;
    objects_.emplace(new_id, TrackedObject(new_id, feature, frame_id));
    printf("[Database] Added new target %d (total: %zu)\n", new_id, objects_.size());
    return new_id;
}

bool ObjectDatabase::updateTarget(int id, const ObjectFeatures& feature, int frame_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = objects_.find(id);
    if (it != objects_.end()) {
        it->second.update(feature, frame_id);
        return true;
    }
    return false;
}

TrackedObject* ObjectDatabase::getTarget(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = objects_.find(id);
    return (it != objects_.end()) ? &it->second : nullptr;
}

void ObjectDatabase::removeTarget(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    objects_.erase(id);
}

std::vector<TrackedObject> ObjectDatabase::getAllActiveTargets(int max_miss_count) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<TrackedObject> active_targets;
    for (const auto& pair : objects_) {
        if (pair.second.miss_count <= max_miss_count) {
            active_targets.push_back(pair.second);
        }
    }
    return active_targets;
}

void ObjectDatabase::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    objects_.clear();
    next_id_ = 1;
}

void ObjectDatabase::cleanupOldTargets(int max_miss_frames, float min_confidence) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<int> to_remove;
    for (const auto& pair : objects_) {
        const TrackedObject& obj = pair.second;
        if (obj.miss_count > max_miss_frames || obj.confidence < min_confidence) {
            to_remove.push_back(pair.first);
        }
    }
        
    for (int id : to_remove) {
        objects_.erase(id);
        printf("[Database] Removed old target %d\n", id);
    }
}

TrackedObject* ObjectDatabase::getTargetByPosition(const cv::Point2f& position, float radius) {
    std::lock_guard<std::mutex> lock(mutex_);
    TrackedObject* nearest_target = nullptr;
    float min_distance = std::numeric_limits<float>::max();
        
    for (auto& pair : objects_) {
        TrackedObject& obj = pair.second;
        if (obj.miss_count > 0) continue;
            
        cv::Point2f obj_center = obj.getObjectCenter();
        float dist = cv::norm(obj_center - position);
        
        if (dist < radius && dist < min_distance) {
            min_distance = dist;
            nearest_target = &obj;
        }
    }
        
    return nearest_target;
}