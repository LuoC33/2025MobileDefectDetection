#ifndef TEMPORALFILTER_H
#define TEMPORALFILTER_H

//#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include "utils.h"
#include "SimpleKalmanFilter.h"

class TemporalFilter;

struct Match {
    size_t box_idx;   // 目标框的索引
    size_t track_idx; // 轨迹的索引
};

// 时间过滤类
class TemporalFilter {
public:
    struct Track {
        int id;
        SimpleKalmanFilter kf;
        cv::Rect last_bbox;
        int age = 0;

        // 构造函数，初始化 SimpleKalmanFilter
        Track() : kf(4, 4) {} // 状态维度和测量维度均为 4
    };

    friend std::vector<Match> hungarian_match(const std::vector<YOLOBbox>& boxes, const std::vector<Track>& tracks);

    void update(const std::vector<YOLOBbox>& boxes);

private:
    std::vector<Track> tracks;
};

std::vector<Match> hungarian_match(const std::vector<YOLOBbox>& boxes, const std::vector<TemporalFilter::Track>& tracks);
cv::Mat measurement(const YOLOBbox& box);

#endif