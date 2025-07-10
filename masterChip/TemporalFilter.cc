#include "TemporalFilter.h"

// 定义 measurement 函数
cv::Mat measurement(const YOLOBbox& box) {
    // 将 YOLOBbox 转换为卡尔曼滤波器所需的测量值
    cv::Mat measurement = cv::Mat::zeros(4, 1, CV_32F); // 假设测量值是 4x1 的矩阵
    measurement.at<float>(0) = box.box.x;               // x 坐标
    measurement.at<float>(1) = box.box.y;               // y 坐标
    measurement.at<float>(2) = box.box.width;           // 宽度
    measurement.at<float>(3) = box.box.height;          // 高度
    return measurement;
}

void TemporalFilter::update(const std::vector<YOLOBbox>& boxes) {
    // 预测阶段
    for (auto& t : tracks) {
        cv::Mat prediction = t.kf.predict();
        t.last_bbox = cv::Rect(prediction.at<float>(0), prediction.at<float>(1), prediction.at<float>(2), prediction.at<float>(3));
    }

    // 数据关联（匈牙利算法匹配）
    auto matches = hungarian_match(boxes, tracks);

    // 更新阶段
    for (auto& m : matches) {
        tracks[m.track_idx].kf.correct(measurement(boxes[m.box_idx]));
        tracks[m.track_idx].age = 0;
    }

    // 移除旧轨迹
    tracks.erase(std::remove_if(tracks.begin(), tracks.end(),
        [](const Track& t) { return t.age > 5; }), tracks.end());

    // 增加所有轨迹的年龄
    for (auto& t : tracks) {
        t.age++;
    }

    // 创建新的 Track 对象
    for (const auto& box : boxes) {
        bool matched = false;
        for (const auto& m : matches) {
            if (box == boxes[m.box_idx]) {
                matched = true;
                break;
            }
        }
        if (!matched) {
            Track new_track; // 调用 Track 的构造函数，初始化 SimpleKalmanFilter
            new_track.last_bbox = box.box;
            tracks.push_back(new_track);
        }
    }
}

// 计算两个矩形框的 IoU（交并比）
float calculate_iou(const cv::Rect& box1, const cv::Rect& box2) {
    float inter_area = (box1 & box2).area();
    float union_area = (box1 | box2).area();
    return inter_area / union_area;
}

// 匈牙利算法匹配函数
std::vector<Match> hungarian_match(const std::vector<YOLOBbox>& boxes, const std::vector<TemporalFilter::Track>& tracks) {
    std::vector<Match> matches;

    // 遍历所有目标框和轨迹，计算 IoU 并匹配
    for (size_t i = 0; i < boxes.size(); ++i) {
        float best_iou = 0.5; // 设置 IoU 阈值
        size_t best_track_idx = -1;

        for (size_t j = 0; j < tracks.size(); ++j) {
            float iou = calculate_iou(boxes[i].box, tracks[j].last_bbox);
            if (iou > best_iou) {
                best_iou = iou;
                best_track_idx = j;
            }
        }

        if (best_track_idx != -1) {
            matches.push_back({i, best_track_idx});
        }
    }

    return matches;
}