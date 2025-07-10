#ifndef _ORB_EXTRACTOR_H
#define _ORB_EXTRACTOR_H

#include <opencv2/flann/flann.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <vector>
#include <memory>

/**
 * @brief ORB特征提取器
 */
class ORBExtractor {
public:
    /**
     * @brief 构造函数
     * @param max_features       最大特征点数量
     * @param scale_factor       金字塔缩放因子
     * @param n_levels           金字塔层数
     * @param fast_threshold     FAST角点阈值
     * @param debug_mode         调试模式
     */
    ORBExtractor(int max_features = 1000, float scale_factor = 1.2f, int n_levels = 8,int fast_threshold = 20,int debug_mode = 0);
    
    /**
     * @brief 执行ORB特征提取
     * @param image        输入图像(CV_8UC1)
     * @param keypoints    输出关键点(与SuperPoint格式相同)
     * @param descriptors  输出描述符(与SuperPoint维度相同256维)
     */
    void extract(const cv::Mat& image, std::vector<cv::Point2f>& keypoints,cv::Mat& descriptors);
    
    /**
     * @brief 融合SuperPoint和ORB特征
     * @param sp_keypoints       SuperPoint关键点
     * @param sp_descriptors     SuperPoint描述符
     * @param orb_keypoints      ORB关键点
     * @param orb_descriptors    ORB描述符
     * @param fused_keypoints    融合后关键点
     * @param fused_descriptors  融合后描述符
     * @param min_distance       特征点最小间距(像素)
     */
    static void fuse_features(const std::vector<cv::Point2f>& sp_keypoints,
                            const cv::Mat& sp_descriptors,
                            const std::vector<cv::Point2f>& orb_keypoints,
                            const cv::Mat& orb_descriptors,
                            std::vector<cv::Point2f>& fused_keypoints,
                            cv::Mat& fused_descriptors,
                            float min_distance = 5.0f);

private:
    // ORB检测器实例
    cv::Ptr<cv::ORB> orb_detector_;
    
    // 配置参数
    int max_features_;
    float scale_factor_;
    int n_levels_;
    int fast_threshold_;
    int debug_mode_;
    
    // 内部缓冲区(线程安全)
    std::vector<cv::KeyPoint> orb_keypoints_;
    cv::Mat orb_descriptors_;
};

#endif // _ORB_EXTRACTOR_H