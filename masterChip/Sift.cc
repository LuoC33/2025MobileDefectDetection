#include "Sift.h"

SIFT::SIFT() : isRunning(false) {}

SIFT::~SIFT() 
{
    stop();
}

// 启动 SIFT 线程
void SIFT::start() {
    isRunning = true;
    workerThread = std::thread(&SIFT::run, this);
}

// 停止 SIFT 线程
void SIFT::stop() {
    isRunning = false;
    queueCond.notify_all(); // 唤醒线程
    if (workerThread.joinable()) {
        workerThread.join();
    }
}

// 将分割的图像放入队列
void SIFT::enqueueImage(const cv::Mat& image, int label) {
    std::lock_guard<std::mutex> lock(queueMutex);
    taskQueue.push({image, label});
    queueCond.notify_one(); // 唤醒线程
}

// SIFT 线程函数
void SIFT::run() {
    while (isRunning) {
        Task task;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCond.wait(lock, [this] { return !taskQueue.empty() || !isRunning; });

            if (!isRunning) break;

            task = taskQueue.front();
            taskQueue.pop();
        }

        // 提取特征点
        std::vector<cv::KeyPoint> keypoints;
        cv::Mat descriptors;
        extractFeatures(task.image, keypoints, descriptors);

        // 匹配并更新历史信息
        bool isSameTarget = matchAndUpdate(descriptors, task.label);
    }
}

//FLANN进行特征匹配
float SIFT::flannMatch(const cv::Mat& descriptors1, const cv::Mat& descriptors2) {
    cv::FlannBasedMatcher matcher;
    std::vector<cv::DMatch> matches;
    matcher.match(descriptors1, descriptors2, matches);

    // 计算最小距离
    float minDist = std::numeric_limits<float>::max();
    for (const auto& match : matches) {
        if (match.distance < minDist) minDist = match.distance;
    }

    // 筛选出好的匹配点
    std::vector<cv::DMatch> goodMatches;
    for (const auto& match : matches) {
        if (match.distance <= std::max(2 * minDist, 0.02f)) {
            goodMatches.push_back(match);
        }
    }

    // 计算相似度概率
    float similarity = static_cast<float>(goodMatches.size()) / matches.size();
    return similarity;
}

//提取缺陷特征点
void SIFT::extractFeatures(const cv::Mat& image, std::vector<cv::KeyPoint>& keypoints, cv::Mat& descriptors) {
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);

    // 调整 SIFT 参数
    cv::Ptr<cv::SIFT> sift = cv::SIFT::create(
        150,        // nFeatures: 0 表示不限制数量
        3,        // nOctaveLayers: 金字塔层数
        0.04,     // contrastThreshold: 提高对比度阈值，过滤低对比度点
        10,       // edgeThreshold: 提高边缘阈值，过滤边缘点
        1.6       // sigma: 高斯模糊参数
    );
    sift->detectAndCompute(gray, cv::noArray(), keypoints, descriptors);

    // 非极大值抑制
    if (keypoints.size() > 120) { 
        std::sort(keypoints.begin(), keypoints.end(), [](const cv::KeyPoint& a, const cv::KeyPoint& b) {
            return a.response > b.response; 
        });
        keypoints.resize(120); 
    }

    // 统计特征点数量
    int numKeypoints = keypoints.size();
    std::cout << "Extracted " << numKeypoints << " keypoints from image." << std::endl;
}

// 匹配特征点并更新历史信息
bool SIFT::matchAndUpdate(const cv::Mat& descriptors, int label) {
    std::lock_guard<std::mutex> lock(mtx);

    // 获取对应标签的历史特征点队列
    auto& historyQueue = historicalFeaturesMap[label];

    // 如果历史特征点为空，直接保存
    if (historyQueue.empty()) {
        historyQueue.push({descriptors.clone(), label});
        return false; // 新目标
    }

    // 与历史特征点进行匹配
    bool isSameTarget = false;
    for (int i = 0; i < historyQueue.size(); ++i) {
        const auto& historical = historyQueue.front();
        float similarity = flannMatch(descriptors, historical.descriptors);

        if (similarity > 0.7) { // 相似度阈值
            isSameTarget = true;
            break;
        }

        // 将历史特征点重新加入队列
        historyQueue.push(historical);
        historyQueue.pop();
    }

    // 如果不是同一个目标，则保存新特征点
    if (!isSameTarget) {
        if (historyQueue.size() >= 3) { // 只保留最近的 3 个特征点
            historyQueue.pop();
        }
        historyQueue.push({descriptors.clone(), label});
    }

    return isSameTarget;
}

// 标签名称映射
const std::unordered_map<int, std::string> labelMap = {
    {0, "jingliang"},
    {1, "shanzha"},
    {2, "laoda"},
    {3, "guolicheng"},
    {4, "heqizheng"},
    {5, "ningmeng"},
    {6, "moli"}
};