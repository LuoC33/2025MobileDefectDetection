#ifndef YOLO_COMPRESS_H
#define YOLO_COMPRESS_H

#include <vector>
#include <cstdint>
#include "utils.h"

// 压缩后的数据结构
#pragma pack(push, 1)
struct CompressedBbox {
    uint16_t x;         // 量化坐标 (0-65535)
    uint16_t y;
    uint16_t width;
    uint16_t height;
    uint16_t confidence; // 量化置信度 (0-65535 对应 0.0-1.0)
    uint8_t index;       // 目标类别索引
    uint8_t ratio;       // 掩码占比 (0-255 对应 0.0-1.0)
};
#pragma pack(pop)

// 压缩函数
void compress_yolo_data(
    const std::vector<YOLOBbox>& bboxes, 
    uint8_t* output_buffer, 
    size_t* output_size);

// 解压缩函数
void decompress_yolo_data(
    const uint8_t* input_buffer,
    size_t input_size,
    std::vector<YOLOBbox>& output_bboxes);

#endif // YOLO_COMPRESS_H