#include "lyradb/delta_compressor.h"
#include <algorithm>
#include <cstring>

namespace lyradb {
namespace compression {

std::vector<uint8_t> DeltaCompressor::compress(
    const int64_t* values, 
    size_t count) {
    
    if (!values || count == 0) {
        return {};
    }
    
    std::vector<uint8_t> result;
    
    // Store first value (8 bytes)
    int64_t first_val = values[0];
    result.resize(12);  // 8 bytes first_val + 4 bytes count
    std::memcpy(result.data(), &first_val, 8);
    
    uint32_t count_le = count;
    result[8] = count_le & 0xFF;
    result[9] = (count_le >> 8) & 0xFF;
    result[10] = (count_le >> 16) & 0xFF;
    result[11] = (count_le >> 24) & 0xFF;
    
    // Store delta-encoded values
    int64_t prev = first_val;
    for (size_t i = 1; i < count; ++i) {
        int64_t delta = values[i] - prev;
        uint64_t encoded = zigzag_encode(delta);
        
        // Varint encoding (variable-length integer)
        // For simplicity, use 8 bytes per delta (can optimize later)
        uint8_t delta_bytes[8];
        std::memcpy(delta_bytes, &encoded, 8);
        result.insert(result.end(), delta_bytes, delta_bytes + 8);
        
        prev = values[i];
    }
    
    return result;
}

std::vector<int64_t> DeltaCompressor::decompress(
    const uint8_t* data, 
    size_t length) {
    
    if (!data || length < 12) {
        return {};
    }
    
    std::vector<int64_t> result;
    
    // Read first value
    int64_t first_val;
    std::memcpy(&first_val, data, 8);
    
    uint32_t count = 
        data[8] | 
        (data[9] << 8) | 
        (data[10] << 16) | 
        (data[11] << 24);
    
    result.reserve(count);
    result.push_back(first_val);
    
    // Read delta-encoded values
    int64_t current = first_val;
    size_t pos = 12;
    
    for (uint32_t i = 1; i < count; ++i) {
        if (pos + 8 > length) break;
        
        uint64_t encoded;
        std::memcpy(&encoded, data + pos, 8);
        pos += 8;
        
        int64_t delta = zigzag_decode(encoded);
        current += delta;
        result.push_back(current);
    }
    
    return result;
}

double DeltaCompressor::estimate_compression_ratio(
    const int64_t* values, 
    size_t count) {
    
    if (!values || count < 2) {
        return 1.0;
    }
    
    // Sample first values to estimate average delta range
    size_t sample_size = std::min(size_t(1000), count);
    int64_t max_delta = 0;
    int64_t min_delta = 0;
    
    for (size_t i = 1; i < sample_size; ++i) {
        int64_t delta = values[i] - values[i - 1];
        max_delta = std::max(max_delta, delta);
        min_delta = std::min(min_delta, delta);
    }
    
    // If deltas are small, delta encoding is beneficial
    int64_t delta_range = max_delta - min_delta;
    int64_t value_range = values[sample_size - 1] - values[0];
    
    // Compression ratio: smaller deltas = better compression
    double ratio = static_cast<double>(delta_range) / value_range;
    
    // Add header overhead
    ratio += (12.0 + (count * 8)) / (count * 8.0);
    
    return ratio;
}

bool DeltaCompressor::is_suitable(
    const int64_t* values, 
    size_t count) {
    
    if (!values || count < 2) {
        return false;
    }
    
    // Check if data is sorted or nearly sorted
    int sorted_count = 0;
    int64_t prev = values[0];
    
    for (size_t i = 1; i < count; ++i) {
        if (values[i] >= prev) {
            sorted_count++;
        }
        prev = values[i];
    }
    
    // If > 80% of pairs are sorted, suitable for delta encoding
    return sorted_count > (count * 0.8);
}

uint64_t DeltaCompressor::zigzag_encode(int64_t value) {
    return (value << 1) ^ (value >> 63);
}

int64_t DeltaCompressor::zigzag_decode(uint64_t value) {
    return (value >> 1) ^ (-(int64_t)(value & 1));
}

} // namespace compression
} // namespace lyradb
