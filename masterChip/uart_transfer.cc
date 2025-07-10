#include "uart_transfer.h"
#include "yolo_compress.h"
#include <cstring>
#include <chrono>
#include <thread>

UartTransfer::UartTransfer(int uart_id, int tx_pin, int rx_pin, int baud_rate)
    : uart_id_(uart_id), tx_pin_(tx_pin), rx_pin_(rx_pin), 
      baud_rate_(baud_rate), uart_inst_(nullptr) {}

UartTransfer::~UartTransfer() {
    if (uart_inst_) {
        drv_uart_inst_destroy(&uart_inst_);
    }
}

bool UartTransfer::initialize() {
    // 配置FPIOA引脚
    fpioa_func_t tx_func = UART1_TXD;
    fpioa_func_t rx_func = UART1_RXD;
    
    if (uart_id_ == 2) {
        tx_func = UART2_TXD;
        rx_func = UART2_RXD;
    } else if (uart_id_ == 3) {
        tx_func = UART3_TXD;
        rx_func = UART3_RXD;
    }
    
    drv_fpioa_set_pin_func(tx_pin_, tx_func);
    drv_fpioa_set_pin_func(rx_pin_, rx_func);
    
    // 设置引脚属性
    drv_fpioa_set_pin_ds(tx_pin_, 3);  // 中等驱动强度
    drv_fpioa_set_pin_pu(rx_pin_, 1);  // 上拉
    
    // 创建UART实例
    if (drv_uart_inst_create(uart_id_, &uart_inst_) != 0) {
        return false;
    }
    
    // 配置UART参数
    struct uart_configure cfg;
    if (drv_uart_get_config(uart_inst_, &cfg) != 0) {
        return false;
    }
    
    cfg.baud_rate = baud_rate_;
    cfg.data_bits = DATA_BITS_8;
    cfg.stop_bits = STOP_BITS_1;
    cfg.parity    = PARITY_NONE;
    cfg.bit_order = BIT_ORDER_LSB;
    cfg.invert    = NRZ_NORMAL;
    
    if (drv_uart_set_config(uart_inst_, &cfg) != 0) {
        return false;
    }
    
    return true;
}

bool UartTransfer::send_raw_data(const uint8_t* data, size_t size) {
    if (!uart_inst_) {
        printf("UART instance is NULL!\n");
        return false;
    }
    if (size == 0) {
        printf("Attempt to send 0 bytes\n");
        return false;
    }
    printf("Sending %zu bytes from addr %p\n", size, data);
    ssize_t sent = drv_uart_write(uart_inst_, const_cast<uint8_t*>(data), size);
    
    if (sent < 0) {
        perror("drv_uart_write failed");
        printf("Error code: %zd\n", sent);
    }
    return (sent == static_cast<ssize_t>(size));
}

size_t UartTransfer::receive_raw_data(uint8_t* buffer, size_t buffer_size, int timeout_ms) {
    if (!uart_inst_ || buffer_size == 0) return 0;
    auto start = std::chrono::steady_clock::now();
    size_t received = 0;
    while (received < buffer_size) {
        // 检查超时
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
        if (elapsed > timeout_ms) break;
        // 检查数据可用性
        int poll_result = drv_uart_poll(uart_inst_, 10); // 10ms超时
        if (poll_result > 0) {
            ssize_t n = drv_uart_read(uart_inst_, buffer + received, buffer_size - received);
            if (n > 0) {
                received += n;
            }
        } else if (poll_result < 0) {
            // 错误处理
            break;
        }
    }
    
    return received;
}

bool UartTransfer::send_yolo_data(const std::vector<YOLOBbox>& bboxes) {
    // 最大压缩大小: 2字节头 + 255*10字节数据 = 2552字节
    uint8_t buffer[2560];
    size_t compressed_size = 0;
    
    compress_yolo_data(bboxes, buffer, &compressed_size);
    
    if (compressed_size == 0) return false;
    
    return send_raw_data(buffer, compressed_size);
}

bool UartTransfer::receive_yolo_data(std::vector<YOLOBbox>& bboxes, int timeout_ms) {
    // 接收包头
    uint8_t header[2];
    size_t received = receive_raw_data(header, sizeof(header), timeout_ms);
    
    if (received != sizeof(header)) return false;
    
    // 计算完整数据包大小
    const uint8_t num_bboxes = header[0];
    const size_t bbox_size = sizeof(CompressedBbox);
    const size_t packet_size = 2 + num_bboxes * sizeof(CompressedBbox);
    
    // 分配接收缓冲区
    std::vector<uint8_t> packet(packet_size);
    std::memcpy(packet.data(), header, sizeof(header));

    printf("received......\n");

    size_t remaining = packet_size - sizeof(header);

    // 接收剩余数据
    received = receive_raw_data(packet.data() + sizeof(header), remaining, timeout_ms);
    
    if (received != packet_size - sizeof(header)) {
        printf("[Error] Received incomplete data: %zu/%zu bytes\n", 
               received, packet_size - sizeof(header));
        return false;
    }
    
    // 验证数据包大小
    if (packet_size != (2 + num_bboxes * sizeof(CompressedBbox))) {
        printf("[Error] Invalid packet size: %zu (expected %zu)\n",
               packet_size, 2 + num_bboxes * sizeof(CompressedBbox));
        return false;
    }

    // 解压缩数据
    decompress_yolo_data(packet.data(), packet_size, bboxes);
    return !bboxes.empty();
}