#ifndef SIFT_H
#define SIFT_H

#include <opencv2/flann/flann.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <vector>
#include <queue>
#include <mutex>
#include <unordered_map>
#include <condition_variable>
#include <thread>
#include "scoped_timing.hpp"

// 历史特征点信息
struct HistoricalFeatures {
    cv::Mat descriptors; // 特征描述子
    int label;           // 分类标签
};

extern const std::unordered_map<int, std::string> labelMap;

// SIFT特征匹配器
class SIFT {
public:
    SIFT();
    ~SIFT();

    //启动线程
    void start();
    //停止线程
    void stop();

    // 将分割的图像放入队列
    void enqueueImage(const cv::Mat& image, int label);

    const std::unordered_map<int, std::queue<HistoricalFeatures>>& getHistoricalFeaturesMap() const {
        return historicalFeaturesMap;
    }

private:

    //SIFT线程函数
    void run();

    //匹配特征点&&更新历史信息
    bool matchAndUpdate(const cv::Mat& descriptors, int label);

    //提取缺陷特征点
    void extractFeatures(const cv::Mat& image, std::vector<cv::KeyPoint>& keypoints, cv::Mat& descriptors);

    //FLANN进行低性能特征匹配(BFMatcher暴力估计带不动)
    float flannMatch(const cv::Mat& descriptors1, const cv::Mat& descriptors2);


    //线程安全队列
    struct Task {
        cv::Mat image;
        int label;
    };
    std::queue<Task> taskQueue;
    std::mutex queueMutex;
    std::condition_variable queueCond;

    //历史特征点队列（每个标签维护一个队列）
    std::unordered_map<int, std::queue<HistoricalFeatures>> historicalFeaturesMap;
    std::mutex mtx;

    //线程控制
    std::thread workerThread;
    bool isRunning;
};

#endif