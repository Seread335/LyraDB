#include "lyradb/rle_compressor.h"
#include <cstring>
#include <stdexcept>

namespace lyradb {
namespace compression {

std::vector<uint8_t> RLECompressor::compress(
    const uint8_t* data, 
    size_t length, 
    size_t value_size) {
    
    if (!data || length == 0 || value_size == 0) {
        return {};
    }
    
    // Validate length is multiple of value_size
    if (length % value_size != 0) {
        throw std::runtime_error("Data length must be multiple of value_size");
    }
    
    std::vector<uint8_t> result;
    size_t num_values = length / value_size;
    
    // Allocate result with worst-case size
    result.reserve(length + (num_values * 4));
    
    // Analyze and encode runs
    size_t i = 0;
    while (i < num_values) {
        // Count consecutive identical values
        size_t run_count = 1;
        const uint8_t* current_value = data + (i * value_size);
        
        while (i + run_count < num_values) {
            const uint8_t* next_value = data + ((i + run_count) * value_size);
            if (std::memcmp(current_value, next_value, value_size) == 0) {
                run_count++;
            } else {
                break;
            }
        }
        
        // Encode run: [run_count (4 bytes)] [value (value_size bytes)]
        uint32_t count_le = run_count;  // Little-endian encoding
        result.push_back(count_le & 0xFF);
        result.push_back((count_le >> 8) & 0xFF);
        result.push_back((count_le >> 16) & 0xFF);
        result.push_back((count_le >> 24) & 0xFF);
        
        // Append value
        result.insert(result.end(), current_value, current_value + value_size);
        
        i += run_count;
    }
    
    return result;
}

std::vector<uint8_t> RLECompressor::decompress(
    const uint8_t* data, 
    size_t length, 
    size_t value_size) {
    
    if (!data || length == 0 || value_size == 0) {
        return {};
    }
    
    std::vector<uint8_t> result;
    size_t pos = 0;
    
    while (pos < length) {
        if (pos + 4 + value_size > length) {
            throw std::runtime_error("Invalid RLE data");
        }
        
        // Read run count (little-endian)
        uint32_t run_count = 
            data[pos] | 
            (data[pos + 1] << 8) | 
            (data[pos + 2] << 16) | 
            (data[pos + 3] << 24);
        pos += 4;
        
        // Read value
        const uint8_t* value = data + pos;
        pos += value_size;
        
        // Append value run_count times
        for (uint32_t i = 0; i < run_count; ++i) {
            result.insert(result.end(), value, value + value_size);
        }
    }
    
    return result;
}

double RLECompressor::estimate_compression_ratio(
    const uint8_t* data, 
    size_t length, 
    size_t value_size) {
    
    if (!data || length == 0 || value_size == 0) {
        return 1.0;
    }
    
    // Sample first 4KB or entire data
    size_t sample_size = std::min(size_t(4096), length);
    size_t sample_values = sample_size / value_size;
    
    size_t total_run_bytes = 0;
    size_t num_runs = 0;
    
    for (size_t i = 0; i + 1 < sample_values; ++i) {
        const uint8_t* current = data + (i * value_size);
        const uint8_t* next = data + ((i + 1) * value_size);
        
        if (std::memcmp(current, next, value_size) != 0) {
            num_runs++;
        }
    }
    
    if (num_runs == 0) {
        return 1.0;  // All values identical - perfect for RLE
    }
    
    // Estimate: each run takes 4 bytes (count) + value_size
    size_t estimated_compressed = num_runs * (4 + value_size);
    double ratio = static_cast<double>(estimated_compressed) / sample_size;
    
    return ratio;
}

} // namespace compression
} // namespace lyradb
