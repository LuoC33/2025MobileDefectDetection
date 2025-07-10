#ifndef UART_TRANSFER_H
#define UART_TRANSFER_H

#include <cstdint>
#include <vector>
#include "drv_uart.h"
#include "drv_fpioa.h"
#include "utils.h"

class UartTransfer {
public:
    // 构造函数
    UartTransfer(int uart_id, int tx_pin, int rx_pin, int baud_rate = 1500000);
    
    // 析构函数
    ~UartTransfer();
    
    // 初始化UART
    bool initialize();
    
    // 发送原始数据
    bool send_raw_data(const uint8_t* data, size_t size);
    
    // 接收原始数据
    size_t receive_raw_data(uint8_t* buffer, size_t buffer_size, int timeout_ms = 100);
    
    // 发送YOLO数据
    bool send_yolo_data(const std::vector<YOLOBbox>& bboxes);
    
    // 接收YOLO数据
    bool receive_yolo_data(std::vector<YOLOBbox>& bboxes, int timeout_ms = 100);

private:
    int uart_id_;
    int tx_pin_;
    int rx_pin_;
    int baud_rate_;
    drv_uart_inst_t* uart_inst_;
    
    // 禁用拷贝
    UartTransfer(const UartTransfer&) = delete;
    UartTransfer& operator=(const UartTransfer&) = delete;
};

#endif // UART_TRANSFER_H


        // uart.send_yolo_data(yolo_results);
        // printf("send success!");
        // sleep(3);

        // while (initialized) {
        //     std::vector<YOLOBbox> received;
        //     if (uart.receive_yolo_data(received, 100)) {  // 检查返回值
        //         printf("Received %zu bboxes\n", received.size());
        //         for (const auto& bbox : received) {
        //             printf(
        //                 "Detection: \n"
        //                 "  Box: x=%d, y=%d, width=%d, height=%d\n"
        //                 "  Confidence: %.2f\n"
        //                 "  Class Index: %d\n"
        //                 "  Ratio: %.2f\n",
        //                 bbox.box.x, bbox.box.y, bbox.box.width, bbox.box.height,
        //                 bbox.confidence,
        //                 bbox.index,
        //                 bbox.ratio
        //             );
        //             printf("------------------------\n");
        //         }
        //     }
        //     // 短暂休眠
        //     // std::this_thread::sleep_for(std::chrono::milliseconds(1));
        // }