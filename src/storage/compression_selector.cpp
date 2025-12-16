#include "lyradb/compression_selector.h"
#include "lyradb/rle_compressor.h"
#include "lyradb/dict_compressor.h"
#include "lyradb/bitpacking_compressor.h"
#include "lyradb/delta_compressor.h"

namespace lyradb {
namespace compression {

CompressionAlgorithm CompressionSelector::select_for_integers(
    const int64_t* values,
    size_t count,
    double min_compression_ratio) {
    
    if (!values || count == 0) {
        return CompressionAlgorithm::UNCOMPRESSED;
    }
    
    // Track best algorithm and ratio
    CompressionAlgorithm best_algo = CompressionAlgorithm::UNCOMPRESSED;
    double best_ratio = 1.0;
    
    // Try Delta compression (best for sorted data)
    if (DeltaCompressor::is_suitable(values, count)) {
        double ratio = DeltaCompressor::estimate_compression_ratio(values, count);
        if (ratio < best_ratio) {
            best_ratio = ratio;
            best_algo = CompressionAlgorithm::DELTA;
        }
    }
    
    // Try Bitpacking (best for bounded ranges)
    double bp_ratio = BitpackingCompressor::estimate_compression_ratio(values, count);
    if (bp_ratio < best_ratio) {
        best_ratio = bp_ratio;
        best_algo = CompressionAlgorithm::BITPACKING;
    }
    
    // Use ZSTD as fallback if compression is beneficial
    if (best_ratio > min_compression_ratio) {
        return best_algo;
    }
    
    // Otherwise fall back to ZSTD for general compression
    return CompressionAlgorithm::ZSTD;
}

CompressionAlgorithm CompressionSelector::select_for_binary(
    const uint8_t* data,
    size_t length,
    size_t value_size,
    double min_compression_ratio) {
    
    if (!data || length == 0 || value_size == 0) {
        return CompressionAlgorithm::UNCOMPRESSED;
    }
    
    CompressionAlgorithm best_algo = CompressionAlgorithm::UNCOMPRESSED;
    double best_ratio = 1.0;
    
    // Try RLE (best for repetitive patterns)
    double rle_ratio = RLECompressor::estimate_compression_ratio(data, length, value_size);
    if (rle_ratio < best_ratio) {
        best_ratio = rle_ratio;
        best_algo = CompressionAlgorithm::RLE;
    }
    
    // Check if ratio meets threshold
    if (best_ratio <= min_compression_ratio) {
        return best_algo;
    }
    
    // Fall back to ZSTD
    return CompressionAlgorithm::ZSTD;
}

CompressionAlgorithm CompressionSelector::select_for_strings(
    const std::vector<std::string>& values,
    double min_compression_ratio) {
    
    if (values.empty()) {
        return CompressionAlgorithm::UNCOMPRESSED;
    }
    
    // Dictionary encoding is primary choice for strings
    if (DictionaryCompressor::is_suitable(values, DICT_CARDINALITY_THRESHOLD)) {
        double ratio = DictionaryCompressor::estimate_compression_ratio(values);
        if (ratio <= min_compression_ratio) {
            return CompressionAlgorithm::DICTIONARY;
        }
    }
    
    // Fall back to ZSTD for high-cardinality strings
    return CompressionAlgorithm::ZSTD;
}

const char* CompressionSelector::algorithm_name(CompressionAlgorithm algo) {
    switch (algo) {
        case CompressionAlgorithm::UNCOMPRESSED:
            return "Uncompressed";
        case CompressionAlgorithm::RLE:
            return "Run-Length Encoding";
        case CompressionAlgorithm::DICTIONARY:
            return "Dictionary Encoding";
        case CompressionAlgorithm::BITPACKING:
            return "Bitpacking";
        case CompressionAlgorithm::DELTA:
            return "Delta Encoding";
        case CompressionAlgorithm::ZSTD:
            return "ZSTD";
        default:
            return "Unknown";
    }
}

double CompressionSelector::estimate_ratio(
    CompressionAlgorithm algo,
    const uint8_t* data,
    size_t length,
    size_t value_size) {
    
    if (!data || length == 0) {
        return 1.0;
    }
    
    switch (algo) {
        case CompressionAlgorithm::RLE:
            if (value_size == 0) return 1.0;
            return RLECompressor::estimate_compression_ratio(data, length, value_size);
            
        case CompressionAlgorithm::BITPACKING: {
            // For bitpacking, need to interpret as int64_t array
            if (length % sizeof(int64_t) != 0) return 1.0;
            auto* values = reinterpret_cast<const int64_t*>(data);
            size_t count = length / sizeof(int64_t);
            return BitpackingCompressor::estimate_compression_ratio(values, count);
        }
            
        case CompressionAlgorithm::DELTA: {
            // For delta, need to interpret as int64_t array
            if (length % sizeof(int64_t) != 0) return 1.0;
            auto* values = reinterpret_cast<const int64_t*>(data);
            size_t count = length / sizeof(int64_t);
            return DeltaCompressor::estimate_compression_ratio(values, count);
        }
            
        case CompressionAlgorithm::UNCOMPRESSED:
        case CompressionAlgorithm::ZSTD:
        case CompressionAlgorithm::DICTIONARY:
        default:
            return 1.0;
    }
}

} // namespace compression
} // namespace lyradb
