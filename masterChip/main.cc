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
#include <queue>
#include <atomic>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <mutex>
#include <condition_variable>
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
#include "uart_transfer.h"
#include "yolo_compress.h"

using std::string;
using std::vector;
using namespace nncase;
using namespace nncase::runtime;
using namespace nncase::runtime::k230;
using namespace nncase::F::k230;

std::atomic<bool> isp_stop(true);
std::atomic<bool> tcp_running(true);
std::atomic<bool> yolo_thread_running(false);
std::mutex mtx;
std::condition_variable conva;
UartTransfer* uart = nullptr;
GeneralConfig general_config;
YoloConfig yolo_config;

enum Command {
    HEARTBEAT = 0x00,  // 心跳包
    START_YOLO = 0x01,  // 启动YOLO任务
    DATA_YOLO = 0x02,
    STOP_YOLO  = 0x03,  // 停止YOLO任务
    EXIT_APP   = 0x04   // 退出应用程序
};

// 函数定义：yolo_video_inference，用于执行视频推理
int yolo_video_inference(GeneralConfig &general_config,YoloConfig &yolo_config){
    // 创建一个FrameSize对象，用于存储图像的宽度和高度
    FrameSize image_wh={general_config.AI_FRAME_WIDTH,general_config.AI_FRAME_HEIGHT};
    // 从标签文件中读取标签
    std::vector<std::string> labels=readLabelsFromTxt(yolo_config.labels_txt_filepath);
    // 创建一个DumpRes对象，用于存储帧数据
    DumpRes dump_res;
    // 创建一个空的向量，用于存储YOLO检测结果
    std::vector<YOLOBbox> yolo_results;
    // 创建一个空的Mat对象，用于存储绘制的帧
    cv::Mat draw_frame(general_config.OSD_HEIGHT, general_config.OSD_WIDTH, CV_8UC4, cv::Scalar(0, 0, 0, 0));
    // 创建一个维度向量，用于定义输入张量的形状
    dims_t in_shape { 1, general_config.AI_FRAME_CHANNEL, general_config.AI_FRAME_HEIGHT, general_config.AI_FRAME_WIDTH };
    // 创建一个运行时张量，用于存储输入数据
    runtime_tensor input_tensor;
    // 创建一个PipeLine对象，用于处理视频流
    PipeLine pl(general_config,yolo_config.debug_mode);
    // 初始化PipeLine对象
    pl.Create();

    // // 创建 SIFT 对象
    // SIFT sift;
    // sift.start(); // 启动 SIFT 线程
    // std::cout << "SIFT init successed" << std::endl;
    if(strcmp(yolo_config.model_type, "yolov8") == 0){
        // 创建一个Yolov8对象，用于执行yolov8的推理流程
        Yolov8 yolov8(yolo_config.task_type,yolo_config.task_mode,yolo_config.kmodel_path,yolo_config.conf_thres,yolo_config.nms_thres,yolo_config.mask_thres,labels,image_wh,yolo_config.debug_mode);  
        // 循环执行推理流程，直到isp_stop为true
        while(!isp_stop){
            // 创建一个ScopedTiming对象，用于计算总时间
            ScopedTiming st("total time", 1);
            // 从PipeLine中获取一帧数据
            pl.GetFrame(dump_res);
            // 创建目标 Mat 对象
            //cv::Mat rawFrame(general_config.AI_FRAME_HEIGHT, general_config.AI_FRAME_WIDTH, CV_8UC3);
            // 获取源数据指针
            // uint8_t* src = reinterpret_cast<uint8_t*>(dump_res.virt_addr);
            // // 将 RGB888 Planar 数据拆分为三个通道
            // cv::Mat r_plane(general_config.AI_FRAME_HEIGHT, general_config.AI_FRAME_WIDTH, CV_8UC1, src);
            // cv::Mat g_plane(general_config.AI_FRAME_HEIGHT, general_config.AI_FRAME_WIDTH, CV_8UC1, src + general_config.AI_FRAME_HEIGHT * general_config.AI_FRAME_WIDTH);
            // cv::Mat b_plane(general_config.AI_FRAME_HEIGHT, general_config.AI_FRAME_WIDTH, CV_8UC1, src + 2 * general_config.AI_FRAME_HEIGHT * general_config.AI_FRAME_WIDTH);
            // // 合并三个通道为 BGR888
            // std::vector<cv::Mat> planes = {b_plane, g_plane, r_plane};
            // cv::merge(planes, rawFrame);
            // 创建一个运行时张量，用于存储输入数据
            input_tensor = host_runtime_tensor::create(typecode_t::dt_uint8, in_shape, { (gsl::byte *)dump_res.virt_addr, compute_size(in_shape) },false, hrt::pool_shared, dump_res.phy_addr).expect("cannot create input tensor");
            // 将输入张量的数据同步到设备上
            hrt::sync(input_tensor, sync_op_t::sync_write_back, true).expect("sync write_back failed");
            // 执行预处理
            yolov8.pre_process(input_tensor);
            // 执行推理
            yolov8.inference();
            // 执行后处理
            yolov8.post_process(yolo_results);

            // 通过串口发送YOLO检测结果给第二个K230
            if (uart && !yolo_results.empty()) {
                ScopedTiming st("uart time", 1);
                uint8_t start_cm = DATA_YOLO;
                uart->send_raw_data(&start_cm, sizeof(start_cm));
                uart->send_yolo_data(yolo_results);
                if (yolo_config.debug_mode) {
                    std::cout << "Sent " << yolo_results.size() << " detections via UART" << std::endl;
                }
            }
            // 处理每个检测结果
            // for (const auto& bbox : yolo_results) {
            //     // 调整掩码尺寸以匹配 AI 分辨率
            //     cv::Mat resized_mask;
            //     cv::resize(bbox.mask, resized_mask, rawFrame.size(), 0, 0, cv::INTER_NEAREST);
            //     resized_mask.convertTo(resized_mask, CV_8UC1, 255.0);
            //     // 确保掩码是二值图像
            //     cv::threshold(resized_mask, resized_mask, 0.8 * 255, 255, cv::THRESH_BINARY);
            //     cv::Mat targetImage = extractTargetFromMask(rawFrame, resized_mask);
            //     // 将分割的图像传递给 SIFT 线程
            //     sift.enqueueImage(targetImage, bbox.index);
            // }
    
            // 将绘制的帧设置为黑色
            // draw_frame.setTo(cv::Scalar(0, 0, 0, 0));
            // if(general_config.DISPLAY_MODE==1){
            //     cv::rotate(draw_frame, draw_frame, cv::ROTATE_90_COUNTERCLOCKWISE);
            //     // 在绘制的帧上绘制检测结果
            //     yolov8.draw_results(draw_frame,yolo_results);
            //     cv::rotate(draw_frame, draw_frame, cv::ROTATE_90_CLOCKWISE);
            // }else{
            //     // 在绘制的帧上绘制检测结果
            //     yolov8.draw_results(draw_frame,yolo_results);
            // }
            // // 将绘制的帧插入到PipeLine中
            // pl.InsertFrame(draw_frame.data);
            // // 释放帧数据
            pl.ReleaseFrame();
            yolo_results.clear();
        }
    }
    else{
        std::cout << "仅支持模型: yolov5/yolov8/yolo11 " << std::endl;
        // 销毁PipeLine对象
        pl.Destroy();
        return -1;
    }
    
    // 停止 SIFT 线程
    //sift.stop();
    // 销毁PipeLine对象
    pl.Destroy();
    return 0;
}

// TCP客户端函数
void tcp_client_task(const string& server_ip, int server_port) {
    constexpr int RECONNECT_DELAY = 2000;
    
    while (tcp_running) {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("socket creation failed");
            std::this_thread::sleep_for(std::chrono::milliseconds(RECONNECT_DELAY));
            continue;
        }

        sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_port);
        
        if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
            perror("invalid address");
            close(sockfd);
            std::this_thread::sleep_for(std::chrono::milliseconds(RECONNECT_DELAY));
            continue;
        }

        if (connect(sockfd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            perror("connection failed");
            close(sockfd);
            std::this_thread::sleep_for(std::chrono::milliseconds(RECONNECT_DELAY));
            continue;
        }

        std::cout << "Connected to TCP server at " << server_ip << ":" << server_port << std::endl;
        
        // 最后心跳时间
        // auto last_heartbeat = std::chrono::steady_clock::now();
        // constexpr int HEARTBEAT_TIMEOUT = 5000; // 5秒超时
        
        while (tcp_running) {
            // 检查心跳超时
            // auto now = std::chrono::steady_clock::now();
            // if (std::chrono::duration_cast<std::chrono::milliseconds>(
            //     now - last_heartbeat).count() > HEARTBEAT_TIMEOUT) {
            //     std::cerr << "Heartbeat timeout, reconnecting..." << std::endl;
            //     break;
            // }
            
            uint8_t command;
            ssize_t bytes_received = recv(sockfd, &command, sizeof(command), MSG_DONTWAIT);
            
            if (bytes_received > 0) {
                // last_heartbeat = now; // 重置心跳计时
                
                if (yolo_config.debug_mode) {
                    std::cout << "Received command: 0x" << std::hex 
                              << static_cast<int>(command) << std::endl;
                }

                switch (command) {
                    // case HEARTBEAT:
                    //     // 收到心跳，更新计时器
                    //     last_heartbeat = now;
                    //     break;
                        
                    case START_YOLO:
                        if (!isp_stop) {
                            std::cout << "YOLO already running" << std::endl;
                        } else {
                            isp_stop = false;
                            yolo_thread_running = true;
                            std::thread([&] {
                                yolo_video_inference(general_config, yolo_config);
                                yolo_thread_running = false;
                            }).detach();
                            std::cout << "Started YOLO inference" << std::endl;
                            
                            // 通知第二个K230
                            if (uart) {
                                uint8_t start_cmd = START_YOLO;
                                uart->send_raw_data(&start_cmd, sizeof(start_cmd));
                            }
                        }
                        break;
                        
                    case STOP_YOLO:
                        isp_stop = true;
                        std::cout << "Stopping YOLO inference" << std::endl;
                        
                        // 等待YOLO线程结束
                        while (yolo_thread_running) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        }
                        
                        // 通知第二个K230
                        if (uart) {
                            uint8_t stop_cmd = STOP_YOLO;
                            uart->send_raw_data(&stop_cmd, sizeof(stop_cmd));
                        }
                        break;
                        
                    case EXIT_APP:
                        tcp_running = false;
                        isp_stop = true;
                        if (uart) {
                            uint8_t exit_cmd = EXIT_APP;
                            uart->send_raw_data(&exit_cmd, sizeof(exit_cmd));
                        }
                        std::cout << "Exiting application" << std::endl;
                        break;
                        
                    default:
                        std::cerr << "Unknown command: 0x" 
                                  << std::hex << static_cast<int>(command) << std::endl;
                        break;
                }
            } else if (bytes_received == 0) {
                std::cout << "TCP connection closed by server" << std::endl;
                break;
            } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("recv failed");
                break;
            }
            
            // 短暂休眠减少CPU占用
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        close(sockfd);
        
        if (tcp_running) {
            std::cout << "Reconnecting in " << RECONNECT_DELAY << "ms..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(RECONNECT_DELAY));
        }
    }
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

// 主函数入口，程序从这里开始执行
int main(int argc, char *argv[])
{
    // 打印程序名称、编译日期和时间
    std::cout << "case " << argv[0] << " build " << __DATE__ << " " << __TIME__ << std::endl;

    std::string server_ip = "192.168.5.20"; // 默认上位机IP
    int server_port = 8080;                // 默认上位机端口

    uart = new UartTransfer(2, 11, 12, 115200); // UART2, TX:11, RX:12
    if (!uart->initialize()) {
        std::cerr << "UART initialization failed!" << std::endl;
        delete uart;
        uart = nullptr;
    } else {
        std::cout << "UART initialized successfully" << std::endl;
    }

    // 遍历命令行参数，解析并设置配置项
    for (int i = 1; i < argc; i += 2)
    {
        if (strcmp(argv[i], "-help") == 0)
        {
            // 打印帮助信息
            _help();
            return 0;
        }
        else if (strcmp(argv[i], "-ai_frame_width") == 0)
        {
            // 设置AI帧宽度
            general_config.AI_FRAME_WIDTH = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-ai_frame_height") == 0)
        {
            // 设置AI帧高度
            general_config.AI_FRAME_HEIGHT = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-display_mode") == 0)
        {
            // 设置显示模式
            general_config.DISPLAY_MODE = atoi(argv[i + 1]);
            if(general_config.DISPLAY_MODE == 0){
                // 设置显示分辨率和旋转角度
                general_config.DISPLAY_WIDTH = 1920;
                general_config.DISPLAY_HEIGHT = 1080;
                general_config.DISPLAY_ROTATE = 0;
            }
            else if(general_config.DISPLAY_MODE == 1){
                // 设置显示分辨率和旋转角度
                general_config.DISPLAY_WIDTH = 800;
                general_config.DISPLAY_HEIGHT = 480;
                general_config.DISPLAY_ROTATE = 1;
                general_config.OSD_WIDTH = 480;
                general_config.OSD_HEIGHT = 800;
            }
            else if(general_config.DISPLAY_MODE == 2){
                // 设置显示分辨率和旋转角度
                general_config.DISPLAY_WIDTH = 1080;
                general_config.DISPLAY_HEIGHT = 1920;
                general_config.DISPLAY_ROTATE = 0;
                general_config.OSD_WIDTH = 1080;
                general_config.OSD_HEIGHT = 1920;
            }
            else{
                // 打印错误信息并退出
                printf("Error :Invalid arguments %s\n", argv[i]);
                _help();
                return -1;
            }
        }
        else if (strcmp(argv[i], "-model_type") == 0)
        {
            // 设置模型类型
            yolo_config.model_type = argv[i + 1];
        }
        else if (strcmp(argv[i], "-task_type") == 0)
        {
            // 设置任务类型
            yolo_config.task_type = argv[i + 1];
        }
        else if (strcmp(argv[i], "-task_mode") == 0)
        {
            // 设置任务模式
            yolo_config.task_mode = argv[i + 1];
        }
        else if (strcmp(argv[i], "-image_path") == 0)
        {
            // 设置图像路径
            yolo_config.image_path = argv[i + 1];
        }
        else if (strcmp(argv[i], "-kmodel_path") == 0)
        {
            // 设置模型路径
            yolo_config.kmodel_path = argv[i + 1];
        }
        else if (strcmp(argv[i], "-labels_txt_filepath") == 0)
        {
            // 设置标签文件路径
            yolo_config.labels_txt_filepath = argv[i + 1];
        }
        else if (strcmp(argv[i], "-conf_thres") == 0)
        {
            // 设置置信度阈值
            yolo_config.conf_thres = atof(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-nms_thres") == 0)
        {
            // 设置非极大值抑制阈值
            yolo_config.nms_thres = atof(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-mask_thres") == 0)
        {
            // 设置掩码阈值
            yolo_config.mask_thres = atof(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-debug_mode") == 0)
        {
            // 设置调试模式
            yolo_config.debug_mode = atoi(argv[i + 1]);
        }
        else
        {
            // 打印错误信息并退出
            printf("Error :Invalid arguments %s\n", argv[i]);
            _help();
            return -1;
        }
    }
        // 启动TCP客户端线程
    std::thread tcp_thread([server_ip, server_port] {
        tcp_client_task(server_ip, server_port);
    });

    // 主循环等待退出信号
    while (tcp_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // 清理资源
    if (tcp_thread.joinable()) {
        tcp_thread.join();
    }

    if (uart) {
        delete uart;
    }
    return 0;
}