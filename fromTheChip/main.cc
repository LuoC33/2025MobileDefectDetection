/**
 ****************************************************************************************************
 * @author      队友的BUG比我的更离谱
 * @version     V1.0
 ****************************************************************************************************
 */
#include <iostream>
#include <thread>
#include <chrono>
#include <fstream>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <nncase/runtime/runtime_tensor.h>
#include <nncase/runtime/runtime_op_utility.h>
#include "pipeline.h"
#include "yolov5.h"
#include "yolov8.h"
#include "yolo11.h"
#include "superpoint.h"
#include "ORBExtractor.h"
#include <rtthread.h>
#include <rtdevice.h>
#include "feature_processor.h"
#include "SpatialFilter.h"
#include "AppearanceFilter.h"
#include "GeometricVerifier.h"
#include "TemporalFilter.h"
#include "ObjectDatabase.h"
#include "MultiLayerFilter.h"
#include "uart_transfer.h"
#include <unistd.h>
#include "network_comm.h"
#include "yolo_compress.h"
#include <sys/mman.h>
#include "rt_wait.h"
#include <fcntl.h>
#include <semaphore.h>
#include <pthread.h>
#include <queue> 
#include <sys/ipc.h>
#include <sys/shm.h>
#include <poll.h>

using std::string;
using std::vector;
using namespace nncase;
using namespace nncase::runtime;
using namespace nncase::runtime::k230;
using namespace nncase::F::k230;

int data_pipe[2]; 
int ctrl_pipe[2]; 

std::atomic<bool> isp_stop(false);
pid_t g_child_pid = -1;
UartTransfer* g_uart = nullptr;
UdpSender* g_udp_sender = nullptr;

cv::Mat create_test_mask(int center_x, int center_y, int size) {
    cv::Mat mask = cv::Mat::zeros(320, 320, CV_8UC1);
    
    cv::ellipse(mask, 
               cv::Point(center_x, center_y),
               cv::Size(size, size),
               0, 0, 360,
               cv::Scalar(255), -1);
    
    return mask;
}

std::vector<YOLOBbox> create_test_yolo_results() {
    std::vector<YOLOBbox> results;
    
    // 目标1：左上区域
    YOLOBbox obj1;
    obj1.box = cv::Rect(40, 40, 120, 120);
    obj1.confidence = 0.95f;
    obj1.index = 0;
    obj1.mask = create_test_mask(150, 150, 120);
    
    // 目标2：右上区域
    YOLOBbox obj2;
    obj2.box = cv::Rect(200, 40, 100, 200);
    obj2.confidence = 0.92f;
    obj2.index = 1;
    obj2.mask = create_test_mask(330, 150, 100);
    
    // 目标3：下方区域
    YOLOBbox obj3;
    obj3.box = cv::Rect(100, 260, 160, 40);
    obj3.confidence = 0.90f;
    obj3.index = 2;
    obj3.mask = create_test_mask(240, 350, 150);
    
    results.push_back(obj1);
    results.push_back(obj2);
    results.push_back(obj3);
    
    return results;
}

// 子进程处理函数
void child_process_task(GeneralConfig &general_config,YoloConfig &yolo_config) {
    close(data_pipe[1]); // 关闭数据管道的写端
    close(ctrl_pipe[1]); // 关闭控制管道的写端
    
    // 设置管道为非阻塞
    fcntl(data_pipe[0], F_SETFL, O_NONBLOCK);
    fcntl(ctrl_pipe[0], F_SETFL, O_NONBLOCK);
    // 初始化UDP发送器
    g_udp_sender = new UdpSender("192.168.5.20", 8081); // 目标IP和端口
    // 创建一个FrameSize对象，用于存储图像的宽度和高度
    FrameSize image_wh={general_config.AI_FRAME_WIDTH,general_config.AI_FRAME_HEIGHT};
    // 创建一个DumpRes对象，用于存储帧数据
    DumpRes dump_res;
    // 创建一个维度向量，用于定义输入张量的形状
    dims_t in_shape { 1, general_config.AI_FRAME_CHANNEL, general_config.AI_FRAME_HEIGHT, general_config.AI_FRAME_WIDTH };
    // 创建一个运行时张量，用于存储输入数据
    runtime_tensor input_tensor;
    // 创建一个PipeLine对象，用于处理视频流
    PipeLine pl(general_config,yolo_config.debug_mode);
    // 初始化PipeLine对象
    pl.Create();
    // 初始化关键点容器
    std::vector<cv::Point2f> keypoints;
    // 初始化描述子容器
    cv::Mat descriptors;
    // 创建Spuerpoint器
    SuperPoint sp(yolo_config.kmodel_path, 0.5f, 4.0f, image_wh, 1);
    // 创建ORB检测器
    ORBExtractor orb_extractor(150, 1.2f, 5, 25, 1);
    // 总处理外壳类初始化
    FeatureProcessor feature_processor;
    // 所有目标的新容器（包含关键点&描述子）
    std::vector<ObjectFeatures> object_features_list;
    // 初始化多层过滤器
    FilterConfig config;
    config.spatial.grid_size = 60;
    config.appearance.vocabulary_size = 500;
    config.geometric.min_inliers = 8;
    config.temporal.max_frame_gap = 7;
    // 创建多层过滤器
    MultiLayerFilter filter(1280, 720, config);
    printf("1\n");
    struct ThreadData {
        std::queue<std::vector<ObjectFeatures>> features_queue;
        pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
        pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;
        std::atomic<bool> exit_thread{false};
        MultiLayerFilter* filter = nullptr;
        UdpSender* udp_sender = nullptr;
    };
    printf("11\n");
    
    ThreadData thread_data;
    thread_data.filter = &filter;
    thread_data.udp_sender = g_udp_sender;
    
    printf("111\n");
    // 创建匹配线程
    auto matching_thread = [](void* arg) -> void* {
        ThreadData* data = static_cast<ThreadData*>(arg);
        int frame_id = 0;

        while (!data->exit_thread) {
            pthread_mutex_lock(&data->queue_mutex);
        
            // 等待队列中有数据或退出信号
            while (data->features_queue.empty() && !data->exit_thread) {
                pthread_cond_wait(&data->queue_cond, &data->queue_mutex);
            }
        
            if (data->exit_thread) {
                pthread_mutex_unlock(&data->queue_mutex);
                break;
            }
          
            // 从队列中取出数据
            auto features_list = std::move(data->features_queue.front());
            data->features_queue.pop();
            pthread_mutex_unlock(&data->queue_mutex);
          
            // 处理数据
            std::vector<int> tracked_ids = data->filter->processFrame(features_list, frame_id++);
            const auto& new_objects = data->filter->getNewObjects();

            // 发送结果
            if (!new_objects.empty()) {
                data->udp_sender->sendObjectData(new_objects);
            }

            // 定期清理旧目标
            if (frame_id % 30 == 0) {
                data->filter->cleanupOldTargets();
            }
        }
        return nullptr;
    };
    printf("1111\n");
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    
    // 设置栈大小（1MB）
    size_t stack_size = 1024 * 1024;
    pthread_attr_setstacksize(&attr, stack_size);
    
    pthread_t thread_id;
    int rc = pthread_create(&thread_id, &attr, matching_thread, &thread_data);
    pthread_attr_destroy(&attr);
    
    if (rc != 0) {
        std::cerr << "Failed to create thread: " << strerror(rc) << std::endl;
        exit(EXIT_FAILURE);
    }
    printf("11111\n");
    // struct sched_param param;
    // param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    // pthread_setschedparam(thread_id, SCHED_FIFO, &param);
    // printf("111111\n");
    // 设置实时优先级
    // param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    // if (sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
    //     perror("sched_setscheduler failed");
    // }
    //printf("1111111\n");
    bool stop_requested = false;

    while(!stop_requested){
        ScopedTiming st("total time", 1);

        // 使用poll检查管道数据，减少延迟
        struct pollfd fds[2];
        fds[0].fd = data_pipe[0];
        fds[0].events = POLLIN;
        fds[1].fd = ctrl_pipe[0];
        fds[1].events = POLLIN;
        
        // 等待管道数据，超时时间1ms
        int ret = poll(fds, 2, 1);
        
        if (ret > 0) {
            // 检查控制管道是否有停止命令
            if (fds[1].revents & POLLIN) {
                uint8_t cmd;
                if (read(ctrl_pipe[0], &cmd, 1) > 0 && cmd == 0xFF) {
                    stop_requested = true;
                    break;
                }
            }
            
            // 检查数据管道是否有YOLO数据
            if (fds[0].revents & POLLIN) {
                size_t count = 0;
                ssize_t n = read(data_pipe[0], &count, sizeof(count));
                if (n == sizeof(count) && count > 0 && count <= 50) {
                    std::vector<YOLOBbox> bboxes(count);
                    n = read(data_pipe[0], bboxes.data(), count * sizeof(YOLOBbox));
                    
                    if (n == static_cast<ssize_t>(count * sizeof(YOLOBbox))) {
                        // 处理接收到的bboxes
                        pl.GetFrame(dump_res);
                        
                        std::vector<ObjectFeatures> object_features_list;
                        feature_processor.process_features(
                            dump_res,
                            bboxes,
                            object_features_list,
                            sp,
                            orb_extractor
                        );

                        pthread_mutex_lock(&thread_data.queue_mutex);
                        thread_data.features_queue.push(std::move(object_features_list));
                        pthread_cond_signal(&thread_data.queue_cond);
                        pthread_mutex_unlock(&thread_data.queue_mutex);

                        pl.ReleaseFrame();
                    }
                }
            }
        } else if (ret == 0) {
            // 超时，没有数据，可以处理其他任务或短暂休眠
            usleep(1000); // 1ms
        }
    }
    
    // 等待停止信号
    thread_data.exit_thread = true;
    pthread_cond_signal(&thread_data.queue_cond);
    pthread_join(thread_id, nullptr);
    
    // 清理资源
    delete g_udp_sender;
    // 关闭管道
    close(data_pipe[0]);
    close(ctrl_pipe[0]);
    pl.Destroy();
    exit(0);
}

// 创建管道
bool init_ipc() {
    // 创建数据管道
    if (pipe(data_pipe)) {
        perror("Data pipe creation failed");
        return false;
    }
    
    // 创建控制管道
    if (pipe(ctrl_pipe)) {
        perror("Control pipe creation failed");
        return false;
    }
    
    // 设置管道为非阻塞
    fcntl(data_pipe[0], F_SETFL, O_NONBLOCK);
    fcntl(data_pipe[1], F_SETFL, O_NONBLOCK);
    fcntl(ctrl_pipe[0], F_SETFL, O_NONBLOCK);
    fcntl(ctrl_pipe[1], F_SETFL, O_NONBLOCK);
    
    // 增大管道缓冲区
    int pipe_size = 1024; // 64KB
    setsockopt(data_pipe[0], SOL_SOCKET, SO_RCVBUF, &pipe_size, sizeof(pipe_size));
    setsockopt(data_pipe[1], SOL_SOCKET, SO_SNDBUF, &pipe_size, sizeof(pipe_size));
    printf("Pipeline true\n");
    return true;
}

// 关闭管道
void destroy_ipc() {
    close(data_pipe[0]);
    close(data_pipe[1]);
    close(ctrl_pipe[0]);
    close(ctrl_pipe[1]);
}

void _help(){
    printf("Please input:\n");
    printf("-ai_frame_width: default 640\n");
    printf("-ai_frame_height: default 360\n");
    printf("-display_mode: default 0,\n");
    printf("   mode 0: LT9611\n");
    printf("   mode 1: ST7701\n");
    printf("   mode 2: HX8377\n");
    printf("-model_type: default yolov8, yolov5/yolov8/yolo11\n");
    printf("-task_type: default detect, classify/detect/segment\n");
    printf("-task_mode: default video, image/video\n");
    printf("-image_path: default test.jpg, image path\n");
    printf("-kmodel_path: default yolov8n.kmodel, kmodel path\n");
    printf("-labels_txt_filepath: default coco_labels.txt, labels txt filepath\n");
    printf("-conf_thres: default 0.35\n");
    printf("-nms_thres: default 0.65\n");
    printf("-mask_thres: default 0.5\n");
    printf("-debug_mode: default 0, 0/1\n");
}

    // YoloConfig yolo_config;

    // // 遍历命令行参数，解析并设置配置项
    // for (int i = 1; i < argc; i += 2)
    // {
    //     if (strcmp(argv[i], "-help") == 0)
    //     {
    //         // 打印帮助信息
    //         _help();
    //         return 0;
    //     }
    //     else if (strcmp(argv[i], "-ai_frame_width") == 0)
    //     {
    //         // 设置AI帧宽度
    //         general_config.AI_FRAME_WIDTH = atoi(argv[i + 1]);
    //     }
    //     else if (strcmp(argv[i], "-ai_frame_height") == 0)
    //     {
    //         // 设置AI帧高度
    //         general_config.AI_FRAME_HEIGHT = atoi(argv[i + 1]);
    //     }
    //     else if (strcmp(argv[i], "-display_mode") == 0)
    //     {
    //         // 设置显示模式
    //         general_config.DISPLAY_MODE = atoi(argv[i + 1]);
    //         if(general_config.DISPLAY_MODE == 0){
    //             // 设置显示分辨率和旋转角度
    //             general_config.DISPLAY_WIDTH = 1920;
    //             general_config.DISPLAY_HEIGHT = 1080;
    //             general_config.DISPLAY_ROTATE = 0;
    //         }
    //         else if(general_config.DISPLAY_MODE == 1){
    //             // 设置显示分辨率和旋转角度
    //             general_config.DISPLAY_WIDTH = 800;
    //             general_config.DISPLAY_HEIGHT = 480;
    //             general_config.DISPLAY_ROTATE = 1;
    //             general_config.OSD_WIDTH = 480;
    //             general_config.OSD_HEIGHT = 800;
    //         }
    //         else if(general_config.DISPLAY_MODE == 2){
    //             // 设置显示分辨率和旋转角度
    //             general_config.DISPLAY_WIDTH = 1080;
    //             general_config.DISPLAY_HEIGHT = 1920;
    //             general_config.DISPLAY_ROTATE = 0;
    //             general_config.OSD_WIDTH = 1080;
    //             general_config.OSD_HEIGHT = 1920;
    //         }
    //         else{
    //             // 打印错误信息并退出
    //             printf("Error :Invalid arguments %s\n", argv[i]);
    //             _help();
    //             return -1;
    //         }
    //     }
    //     else if (strcmp(argv[i], "-model_type") == 0)
    //     {
    //         // 设置模型类型
    //         yolo_config.model_type = argv[i + 1];
    //     }
    //     else if (strcmp(argv[i], "-task_type") == 0)
    //     {
    //         // 设置任务类型
    //         yolo_config.task_type = argv[i + 1];
    //     }
    //     else if (strcmp(argv[i], "-task_mode") == 0)
    //     {
    //         // 设置任务模式
    //         yolo_config.task_mode = argv[i + 1];
    //     }
    //     else if (strcmp(argv[i], "-image_path") == 0)
    //     {
    //         // 设置图像路径
    //         yolo_config.image_path = argv[i + 1];
    //     }
    //     else if (strcmp(argv[i], "-kmodel_path") == 0)
    //     {
    //         // 设置模型路径
    //         yolo_config.kmodel_path = argv[i + 1];
    //     }
    //     else if (strcmp(argv[i], "-labels_txt_filepath") == 0)
    //     {
    //         // 设置标签文件路径
    //         yolo_config.labels_txt_filepath = argv[i + 1];
    //     }
    //     else if (strcmp(argv[i], "-conf_thres") == 0)
    //     {
    //         // 设置置信度阈值
    //         yolo_config.conf_thres = atof(argv[i + 1]);
    //     }
    //     else if (strcmp(argv[i], "-nms_thres") == 0)
    //     {
    //         // 设置非极大值抑制阈值
    //         yolo_config.nms_thres = atof(argv[i + 1]);
    //     }
    //     else if (strcmp(argv[i], "-mask_thres") == 0)
    //     {
    //         // 设置掩码阈值
    //         yolo_config.mask_thres = atof(argv[i + 1]);
    //     }
    //     else if (strcmp(argv[i], "-debug_mode") == 0)
    //     {
    //         // 设置调试模式
    //         yolo_config.debug_mode = atoi(argv[i + 1]);
    //     }
    //     else
    //     {
    //         // 打印错误信息并退出
    //         printf("Error :Invalid arguments %s\n", argv[i]);
    //         _help();
    //         return -1;
    //     }
    // }

int main(int argc, char *argv[]) {
    YoloConfig yolo_config;
    // 初始化UART
    g_uart = new UartTransfer(2, 11, 12, 115200);
    if (!g_uart->initialize()) {
        std::cerr << "UART initialization failed!" << std::endl;
        return -1;
    }
    
    // 初始化管道
    if (!init_ipc()) {
        std::cerr << "IPC initialization failed!" << std::endl;
        return -1;
    }

    while (true) {
        uint8_t header;
        ssize_t received = g_uart->receive_raw_data(&header, 1, 100);
        
        if (received == 1) {
            std::cout << "Received header: 0x" << std::hex << (int)header << std::dec << std::endl;
            
            // 命令处理 (无论子进程状态)
            if (header == 0x01 && g_child_pid <= 0) {
                // 启动子进程
                g_child_pid = fork();
                if (g_child_pid == 0) {
                    child_process_task(general_config, yolo_config);
                } else if (g_child_pid > 0) {
                    close(data_pipe[0]);
                    close(ctrl_pipe[0]);
                    std::cout << "Started task PID: " << g_child_pid << std::endl;
                }
            } 
            else if (header == 0x04) {
                // 退出命令
                if (g_child_pid > 0) {
                    uint8_t stop_cmd = 0xFF;
                    write(ctrl_pipe[1], &stop_cmd, 1);
                    waitpid(g_child_pid, nullptr, 0);
                }
                break;
            }
            // YOLO数据处理
            else if (header == 0x02 && g_child_pid > 0) {
                std::vector<YOLOBbox> bboxes;
                if (g_uart->receive_yolo_data(bboxes, 200)) {  // 增加超时
                    size_t count = bboxes.size();
                    if (count > 20) count = 20;
                    
                    // 写入数据管道
                    write(data_pipe[1], &count, sizeof(count));
                    write(data_pipe[1], bboxes.data(), count * sizeof(YOLOBbox));
                }
            }
            // 停止命令
            else if (header == 0x03 && g_child_pid > 0) {
                uint8_t stop_cmd = 0xFF;
                write(ctrl_pipe[1], &stop_cmd, 1);
                waitpid(g_child_pid, nullptr, 0);
                g_child_pid = -1;
                std::cout << "Task stopped" << std::endl;
            }
        }
        // 非阻塞等待
        usleep(2000);
    }
    destroy_ipc();
    delete g_uart;
    return 0;
}