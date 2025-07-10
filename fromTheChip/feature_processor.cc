#include "feature_processor.h"
#include <chrono>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

// 全局配置
GeneralConfig general_config;

FeatureProcessor::FeatureProcessor() {
}

FeatureProcessor::~FeatureProcessor() {
}

void FeatureProcessor::apply_boxes(DumpRes& dump_res, const std::vector<YOLOBbox>& yolo_results) {
    if (yolo_results.empty()) return;

    // 直接操作原始内存
    cv::Mat frame(
        general_config.AI_FRAME_HEIGHT, 
        general_config.AI_FRAME_WIDTH,
        CV_8UC1, 
        reinterpret_cast<void*>(dump_res.virt_addr)
    );

    // 创建总掩码,所有框外区域为黑色
    cv::Mat mask = cv::Mat::zeros(frame.size(), CV_8UC1);

    // 在所有边界框内创建白色区域
    for (const auto& bbox : yolo_results) {
        cv::Rect box = bbox.box;
        // 确保边界框在图像范围内
        box.x = std::max(0, box.x);
        box.y = std::max(0, box.y);
        box.width = std::min(box.width, frame.cols - box.x);
        box.height = std::min(box.height, frame.rows - box.y);
        
        // 在掩码上创建矩形区域（白色）
        mask(box) = 255;
    }
    
    // 应用掩码：只保留边界框内的区域
    cv::bitwise_and(frame, mask, frame);
    
    // // 调试可视化 - 每30帧保存一次
    // if (frame_count % 30 == 0) {
    //     // 保存原始帧
    //     cv::imwrite("original_frame_" + std::to_string(frame_count) + ".png", frame);
        
    //     // 创建彩色版本用于可视化
    //     cv::Mat color_frame;
    //     cv::cvtColor(frame, color_frame, cv::COLOR_GRAY2BGR);
        
    //     // 在图像上绘制所有边界框
    //     for (const auto& bbox : yolo_results) {
    //         cv::Rect box = bbox.box;
    //         cv::rectangle(color_frame, box, cv::Scalar(0, 0, 255), 2);
            
    //         // 添加文本标签
    //         std::string label = "Obj " + std::to_string(bbox.index) + ": " 
    //                           + std::to_string(bbox.confidence).substr(0, 4);
    //         cv::putText(color_frame, label, 
    //                    cv::Point(box.x, box.y - 5), 
    //                    cv::FONT_HERSHEY_SIMPLEX, 0.5, 
    //                    cv::Scalar(0, 255, 0), 1);
    //     }
        
    //     // 创建可视化图像
    //     cv::Mat combined(frame.rows, frame.cols * 3, CV_8UC3);
        
    //     // 左侧：原始图像
    //     cv::Mat left_side;
    //     cv::cvtColor(frame, left_side, cv::COLOR_GRAY2BGR);
    //     left_side.copyTo(combined(cv::Rect(0, 0, frame.cols, frame.rows)));
        
    //     // 中间：掩码图像
    //     cv::Mat mask_color;
    //     cv::cvtColor(mask, mask_color, cv::COLOR_GRAY2BGR);
    //     mask_color.copyTo(combined(cv::Rect(frame.cols, 0, frame.cols, frame.rows)));
        
    //     // 右侧：带边界框的图像
    //     color_frame.copyTo(combined(cv::Rect(frame.cols * 2, 0, frame.cols, frame.rows)));
        
    //     // 添加说明文本
    //     cv::putText(combined, "Original Frame", cv::Point(10, 30), 
    //                cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
    //     cv::putText(combined, "Box Mask", cv::Point(frame.cols + 10, 30), 
    //                cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
    //     cv::putText(combined, "With Boxes", cv::Point(frame.cols * 2 + 10, 30), 
    //                cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
    //     cv::putText(combined, "Frame: " + std::to_string(frame_count), 
    //                cv::Point(10, frame.rows - 10), 
    //                cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
        
    //     cv::imwrite("visualization_" + std::to_string(frame_count) + ".png", combined);
    // }

    // frame_count++;
}

void FeatureProcessor::assign_features_to_objects(const std::vector<cv::Point2f>& fused_keypoints,
                                                const cv::Mat& fused_descriptors,
                                                const std::vector<YOLOBbox>& yolo_results,
                                                std::vector<ObjectFeatures>& object_features_list) 
{
    object_features_list.clear();
    if (yolo_results.empty()) return;
    object_features_list.resize(yolo_results.size());
    const cv::Size frame_size(general_config.AI_FRAME_WIDTH, general_config.AI_FRAME_HEIGHT);
    
    // 为每个目标预分配内存
    std::vector<std::vector<cv::Point2f>> temp_keypoints(yolo_results.size());
    std::vector<std::vector<int>> temp_descriptor_indices(yolo_results.size());
    
    // 并行处理特征点 - 使用边界框进行分配
    #pragma omp parallel for
    for (int kp_idx = 0; kp_idx < fused_keypoints.size(); ++kp_idx) {
        const cv::Point2f& kp = fused_keypoints[kp_idx];
        
        // 检查特征点是否在某个边界框内
        for (int obj_idx = 0; obj_idx < yolo_results.size(); ++obj_idx) {
            const auto& box = yolo_results[obj_idx].box;
            
            // 检查点是否在边界框内
            if (kp.x >= box.x && kp.x <= (box.x + box.width) &&
                kp.y >= box.y && kp.y <= (box.y + box.height)) {
                
                #pragma omp critical
                {
                    temp_keypoints[obj_idx].push_back(kp);
                    temp_descriptor_indices[obj_idx].push_back(kp_idx);
                }
                break; // 找到匹配的框后跳出循环
            }
        }
    }
    
    // 组装最终结果
    for (int obj_idx = 0; obj_idx < yolo_results.size(); ++obj_idx) {
        object_features_list[obj_idx].bbox = yolo_results[obj_idx];
        object_features_list[obj_idx].keypoints = std::move(temp_keypoints[obj_idx]);
        
        const auto& indices = temp_descriptor_indices[obj_idx];
        if (!indices.empty()) {
            cv::Mat obj_descriptors(indices.size(), fused_descriptors.cols, fused_descriptors.type());
            
            // 优化描述子复制
            const int desc_size = fused_descriptors.cols * fused_descriptors.elemSize();
            for (int i = 0; i < indices.size(); ++i) {
                const uchar* src = fused_descriptors.ptr(indices[i]);
                uchar* dst = obj_descriptors.ptr(i);
                memcpy(dst, src, desc_size);
            }
            
            object_features_list[obj_idx].descriptors = obj_descriptors;
        }
    }
}

void FeatureProcessor::process_features(DumpRes& dump_res,
                                      const std::vector<YOLOBbox>& yolo_results,
                                      std::vector<ObjectFeatures>& object_features_list,
                                      SuperPoint& sp,
                                      ORBExtractor& orb_extractor) {
    //应用掩码（直接修改原始帧）
    apply_boxes(dump_res, yolo_results);
    
    //准备张量
    dims_t in_shape { 1, general_config.AI_FRAME_CHANNEL, general_config.AI_FRAME_HEIGHT, general_config.AI_FRAME_WIDTH };
    auto input_tensor = host_runtime_tensor::create(
        typecode_t::dt_uint8, 
        in_shape, 
        { (gsl::byte *)dump_res.virt_addr, compute_size(in_shape) },
        false, 
        hrt::pool_shared, 
        dump_res.phy_addr
    ).expect("cannot create input tensor");
    hrt::sync(input_tensor, sync_op_t::sync_write_back, true).expect("sync write_back failed");
    
    // 3并行特征提取
    std::vector<cv::Point2f> sp_keypoints, orb_keypoints, fused_keypoints;
    cv::Mat sp_descriptors, orb_descriptors, fused_descriptors;
    
    // SuperPoint预处理
    sp.pre_process(input_tensor);
    
    // // 启动ORB线程
    // std::thread orb_thread([&](){
    //     cv::Mat gray_frame(
    //         general_config.AI_FRAME_HEIGHT, 
    //         general_config.AI_FRAME_WIDTH,
    //         CV_8UC1, 
    //         reinterpret_cast<void*>(dump_res.virt_addr)
    //     );
    //     orb_extractor.extract(gray_frame, orb_keypoints, orb_descriptors);
    // });
    
    // SuperPoint推理
    sp.inference();
    
    // 等待ORB完成
    // orb_thread.join();
    
    // SuperPoint后处理
    sp.post_process(sp_keypoints, sp_descriptors);
    
    // 特征融合
    // ORBExtractor::fuse_features(
    //     sp_keypoints, sp_descriptors,
    //     orb_keypoints, orb_descriptors,
    //     fused_keypoints, fused_descriptors
    // );
    
    // 特征分配到各个目标
    assign_features_to_objects(
        sp_keypoints, 
        sp_descriptors,
        yolo_results,
        object_features_list
    );
    // // // std::cerr << "111\n" << std::endl;

    // 检查每个目标的特征是否有效
    // for (size_t i = 0; i < object_features_list.size(); ++i) {
    //     const auto& features = object_features_list[i];
    //     if (features.keypoints.empty() || features.descriptors.empty()) {
    //         std::cerr << "Error: Object " << i << " has empty features (keypoints or descriptors)!" << std::endl;
    //     } else {
    //         std::cout << "Success: Object " << i << " has valid features. "
    //                   << "Keypoints: " << features.keypoints.size() << ", "
    //                   << "Descriptors: " << features.descriptors.rows << "x" << features.descriptors.cols << std::endl;
    //     }
    // }
}