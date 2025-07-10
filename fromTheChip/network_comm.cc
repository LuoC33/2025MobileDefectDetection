#include "network_comm.h"
#include "feature_processor.h"

UdpSender::UdpSender(const std::string& ip, int port) {
    sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd_ < 0) {
        perror("socket creation failed");
        return;
    }

    memset(&server_addr_, 0, sizeof(server_addr_));
    server_addr_.sin_family = AF_INET;
    server_addr_.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &server_addr_.sin_addr);
}

UdpSender::~UdpSender() {
    if (sockfd_ >= 0) {
        close(sockfd_);
    }
}

bool UdpSender::sendData(const void* data, size_t size) {
    ssize_t sent = sendto(sockfd_, data, size, 0, 
                         (const struct sockaddr*)&server_addr_, 
                         sizeof(server_addr_));
    return sent == static_cast<ssize_t>(size);
}

bool UdpSender::sendString(const std::string& str) {
    return sendData(str.c_str(), str.size());
}

bool UdpSender::sendObjectData(const std::vector<ObjectFeatures>& objects) {
    // 简化的序列化格式: [数量(4字节)][每个对象数据]
    std::vector<uint8_t> buffer;
    uint32_t count = objects.size();
    
    // 添加对象数量
    buffer.insert(buffer.end(), 
                reinterpret_cast<uint8_t*>(&count),
                reinterpret_cast<uint8_t*>(&count) + sizeof(count));
    
    // 添加每个对象数据
    for (const auto& obj : objects) {
        const cv::Rect& box = obj.bbox.box;
        uint32_t data[6] = {
            static_cast<uint32_t>(box.x),
            static_cast<uint32_t>(box.y),
            static_cast<uint32_t>(box.width),
            static_cast<uint32_t>(box.height),
            static_cast<uint32_t>(obj.keypoints.size()),
            static_cast<uint32_t>(obj.descriptors.rows)
        };
        
        buffer.insert(buffer.end(), 
                    reinterpret_cast<uint8_t*>(data),
                    reinterpret_cast<uint8_t*>(data) + sizeof(data));
    }
    
    return sendData(buffer.data(), buffer.size());
}