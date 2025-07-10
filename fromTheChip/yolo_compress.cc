#include "yolo_compress.h"

void compress_yolo_data(
    const std::vector<YOLOBbox>& bboxes, 
    uint8_t* output_buffer, 
    size_t* output_size) {
    
    // 包头: 目标数量(1字节) + 校验和(1字节)
    const size_t header_size = 2;
    const size_t bbox_size = sizeof(CompressedBbox);
    const size_t max_bboxes = 255;
    
    if (bboxes.size() > max_bboxes) {
        *output_size = 0;
        return;
    }
    
    uint8_t num_bboxes = static_cast<uint8_t>(bboxes.size());
    output_buffer[0] = num_bboxes;
    
    CompressedBbox* compressed = reinterpret_cast<CompressedBbox*>(output_buffer + header_size);
    
    // 压缩每个边界框
    for (size_t i = 0; i < num_bboxes; i++) {
        const YOLOBbox& bbox = bboxes[i];
        
        compressed[i].x = static_cast<uint16_t>(bbox.box.x);
        compressed[i].y = static_cast<uint16_t>(bbox.box.y);
        compressed[i].width = static_cast<uint16_t>(bbox.box.width);
        compressed[i].height = static_cast<uint16_t>(bbox.box.height);
        compressed[i].confidence = static_cast<uint16_t>(bbox.confidence * 65535.0f);
        compressed[i].index = static_cast<uint8_t>(bbox.index);
        compressed[i].ratio = static_cast<uint8_t>(bbox.ratio * 255.0f);
    }
    
    // 计算校验和 (简单XOR校验)
    uint8_t checksum = 0;
    checksum ^= output_buffer[0]; 
    const uint8_t* data_start = output_buffer + 2;
    const size_t data_size = num_bboxes * sizeof(CompressedBbox);
    for (size_t i = 0; i < data_size; i++) {
        checksum ^= data_start[i];
    }
    output_buffer[1] = checksum;
    
    *output_size = header_size + num_bboxes * bbox_size;
    printf("Sending packet (%zu bytes):\n", *output_size);
    for (size_t i = 0; i < *output_size; i++) {
        printf("%02X ", output_buffer[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");
}

void decompress_yolo_data(
    const uint8_t* input_buffer,
    size_t input_size,
    std::vector<YOLOBbox>& output_bboxes) {
    
    output_bboxes.clear();
    
    if (input_size < 2) return;
    
    const uint8_t num_bboxes = input_buffer[0];
    const uint8_t checksum = input_buffer[1];

    printf("Received packet (%zu bytes):\n", input_size);
    for (size_t i = 0; i < input_size; i++) {
        printf("%02X ", input_buffer[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }

    printf("\n");
    // 验证校验和
    uint8_t calc_checksum = 0;
    calc_checksum ^= input_buffer[0];
    const uint8_t* data_start = input_buffer + 2;
    const size_t data_size = num_bboxes * sizeof(CompressedBbox);
    for (size_t i = 0; i < data_size; i++) {
        calc_checksum ^= data_start[i];
    }
    printf("[Debug] num_bboxes=%d, checksum=0x%02X, calc_checksum=0x%02X\n", 
        num_bboxes, checksum, calc_checksum);
    
    if (calc_checksum != checksum) {  // 比较计算值和接收值
        printf("[Error] Checksum mismatch!\n");
        return;
    }

    
    const size_t expected_size = 2 + num_bboxes * sizeof(CompressedBbox);
    //if (input_size < expected_size) return;
    if (input_size < expected_size) {
        printf("[Error] Input size too small: %zu < %zu\n", input_size, expected_size);
        return;
    }
    
    const CompressedBbox* compressed = reinterpret_cast<const CompressedBbox*>(input_buffer + 2);
    output_bboxes.resize(num_bboxes);
    
    // 解压缩每个边界框
    for (size_t i = 0; i < num_bboxes; i++) {
        YOLOBbox& bbox = output_bboxes[i];
        
        bbox.box.x = compressed[i].x;
        bbox.box.y = compressed[i].y;
        bbox.box.width = compressed[i].width;
        bbox.box.height = compressed[i].height;
        bbox.confidence = static_cast<float>(compressed[i].confidence) / 65535.0f;
        bbox.index = compressed[i].index;
        bbox.ratio = static_cast<float>(compressed[i].ratio) / 255.0f;
        
        // 掩码不需要传输
        bbox.mask = cv::Mat();
    }
}