#include "lyradb/bitpacking_compressor.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace lyradb {
namespace compression {

std::vector<uint8_t> BitpackingCompressor::compress(
    const int64_t* values, 
    size_t count) {
    
    if (!values || count == 0) {
        return {};
    }
    
    // Find min and max values
    int64_t min_val = values[0];
    int64_t max_val = values[0];
    
    for (size_t i = 1; i < count; ++i) {
        min_val = std::min(min_val, values[i]);
        max_val = std::max(max_val, values[i]);
    }
    
    // Calculate bits needed for range
    int64_t range = max_val - min_val;
    uint8_t bit_width = calculate_bit_width(range);
    
    // Allocate result buffer
    // Header: 1 (bit_width) + 4 (num_values) + 8 (min_value)
    // Data: ceil(count * bit_width / 8) bytes
    size_t data_bytes = (count * bit_width + 7) / 8;
    std::vector<uint8_t> result(BitpackHeader::SIZE + data_bytes);
    
    // Write header
    result[0] = bit_width;
    uint32_t count_le = count;
    result[1] = count_le & 0xFF;
    result[2] = (count_le >> 8) & 0xFF;
    result[3] = (count_le >> 16) & 0xFF;
    result[4] = (count_le >> 24) & 0xFF;
    
    std::memcpy(result.data() + 5, &min_val, 8);
    
    // Encode values (delta from min, stored in bit_width bits)
    size_t bit_offset = 0;
    uint8_t* data_ptr = result.data() + BitpackHeader::SIZE;
    
    for (size_t i = 0; i < count; ++i) {
        uint64_t delta = static_cast<uint64_t>(values[i] - min_val);
        write_bits(data_ptr, bit_offset, delta, bit_width);
        bit_offset += bit_width;
    }
    
    return result;
}

std::vector<int64_t> BitpackingCompressor::decompress(
    const uint8_t* data, 
    size_t length) {
    
    if (!data || length < BitpackHeader::SIZE) {
        return {};
    }
    
    // Read header
    uint8_t bit_width = data[0];
    uint32_t count = 
        data[1] | 
        (data[2] << 8) | 
        (data[3] << 16) | 
        (data[4] << 24);
    
    int64_t min_val;
    std::memcpy(&min_val, data + 5, 8);
    
    // Decode values
    std::vector<int64_t> result;
    result.reserve(count);
    
    size_t bit_offset = 0;
    const uint8_t* data_ptr = data + BitpackHeader::SIZE;
    size_t data_length = length - BitpackHeader::SIZE;
    
    for (uint32_t i = 0; i < count; ++i) {
        uint64_t delta = read_bits(data_ptr, bit_offset, bit_width);
        result.push_back(min_val + static_cast<int64_t>(delta));
        bit_offset += bit_width;
    }
    
    return result;
}

double BitpackingCompressor::estimate_compression_ratio(
    const int64_t* values, 
    size_t count) {
    
    if (!values || count == 0) {
        return 1.0;
    }
    
    // Find range
    int64_t min_val = values[0];
    int64_t max_val = values[0];
    
    for (size_t i = 1; i < count; ++i) {
        min_val = std::min(min_val, values[i]);
        max_val = std::max(max_val, values[i]);
    }
    
    int64_t range = max_val - min_val;
    uint8_t bit_width = calculate_bit_width(range);
    
    // Original size: 8 bytes * count
    size_t original_size = count * 8;
    
    // Compressed size: header + bitpacked data
    size_t compressed_size = BitpackHeader::SIZE + 
                            ((count * bit_width + 7) / 8);
    
    return static_cast<double>(compressed_size) / original_size;
}

uint8_t BitpackingCompressor::calculate_bit_width(int64_t max_value) {
    if (max_value <= 0) return 0;
    
    uint8_t width = 1;
    int64_t limit = 1;
    
    while (width < 64 && limit < max_value) {
        width++;
        limit = (1LL << width) - 1;
    }
    
    return std::min(width, uint8_t(64));
}

void BitpackingCompressor::write_bits(
    uint8_t* buffer, 
    size_t bit_offset, 
    uint64_t value, 
    uint8_t bit_width) {
    
    for (uint8_t i = 0; i < bit_width; ++i) {
        size_t byte_pos = (bit_offset + i) / 8;
        size_t bit_pos = (bit_offset + i) % 8;
        
        uint8_t bit = (value >> i) & 1;
        if (bit) {
            buffer[byte_pos] |= (1 << bit_pos);
        } else {
            buffer[byte_pos] &= ~(1 << bit_pos);
        }
    }
}

uint64_t BitpackingCompressor::read_bits(
    const uint8_t* buffer, 
    size_t bit_offset, 
    uint8_t bit_width) {
    
    uint64_t value = 0;
    
    for (uint8_t i = 0; i < bit_width; ++i) {
        size_t byte_pos = (bit_offset + i) / 8;
        size_t bit_pos = (bit_offset + i) % 8;
        
        uint8_t bit = (buffer[byte_pos] >> bit_pos) & 1;
        value |= static_cast<uint64_t>(bit) << i;
    }
    
    return value;
}

} // namespace compression
} // namespace lyradb
