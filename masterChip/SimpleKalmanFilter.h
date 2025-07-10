#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>


class SimpleKalmanFilter {
    public:
        SimpleKalmanFilter(int state_dim, int measure_dim) {
            state_.create(state_dim, 1, CV_32F);                  // 状态向量
            measure_.create(measure_dim, 1, CV_32F);              // 测量向量
            transition_matrix_.create(state_dim, state_dim, CV_32F); // 状态转移矩阵
            measurement_matrix_.create(measure_dim, state_dim, CV_32F); // 测量矩阵
            process_noise_cov_.create(state_dim, state_dim, CV_32F); // 过程噪声协方差矩阵
            measurement_noise_cov_.create(measure_dim, measure_dim, CV_32F); // 测量噪声协方差矩阵
            error_cov_post_.create(state_dim, state_dim, CV_32F); // 后验误差协方差矩阵
            error_cov_prior_.create(state_dim, state_dim, CV_32F); // 先验误差协方差矩阵
    
            // 初始化矩阵
            cv::setIdentity(transition_matrix_);
            cv::setIdentity(measurement_matrix_);
            cv::setIdentity(process_noise_cov_, cv::Scalar::all(1e-2));
            cv::setIdentity(measurement_noise_cov_, cv::Scalar::all(1e-1));
            cv::setIdentity(error_cov_post_, cv::Scalar::all(1));
        }
    
        // 预测阶段
        cv::Mat predict() {
            state_ = transition_matrix_ * state_;
            error_cov_prior_ = transition_matrix_ * error_cov_post_ * transition_matrix_.t() + process_noise_cov_;
            return state_;
        }
    
        // 更新阶段
        cv::Mat correct(const cv::Mat& measurement) {
            cv::Mat kalman_gain = error_cov_prior_ * measurement_matrix_.t() *
                                  (measurement_matrix_ * error_cov_prior_ * measurement_matrix_.t() + measurement_noise_cov_).inv();
            state_ = state_ + kalman_gain * (measurement - measurement_matrix_ * state_);
            error_cov_post_ = (cv::Mat::eye(state_.rows, state_.cols, CV_32F) - kalman_gain * measurement_matrix_) * error_cov_prior_;
            return state_;
        }
    
    private:
        cv::Mat state_;                  // 状态向量
        cv::Mat measure_;                // 测量向量
        cv::Mat transition_matrix_;      // 状态转移矩阵
        cv::Mat measurement_matrix_;     // 测量矩阵
        cv::Mat process_noise_cov_;      // 过程噪声协方差矩阵
        cv::Mat measurement_noise_cov_;  // 测量噪声协方差矩阵
        cv::Mat error_cov_post_;         // 后验误差协方差矩阵
        cv::Mat error_cov_prior_;        // 先验误差协方差矩阵
    };
    