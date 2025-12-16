#include "lyradb/zstd_compressor.h"
#include <stdexcept>
#include <algorithm>
#include <cstring>

// ZSTD library headers - try to include if available
#ifdef __has_include
    #if __has_include(<zstd.h>)
        #include <zstd.h>
        #define LYRA_ZSTD_AVAILABLE 1
    #endif
#else
    // For MSVC or older compilers, try including anyway
    #ifdef _WINDOWS
        // On Windows, zstd might not be easily available
        #define LYRA_ZSTD_AVAILABLE 0
    #else
        #include <zstd.h>
        #define LYRA_ZSTD_AVAILABLE 1
    #endif
#endif

namespace lyradb {
namespace compression {

ZstdCompressor::ZstdCompressor(int level) : level_(level) {
    if (level < 1 || level > 22) {
        throw std::runtime_error("ZSTD level must be between 1 and 22");
    }
}

std::vector<uint8_t> ZstdCompressor::compress(
    const uint8_t* data, 
    size_t length) const {
    
    if (!data || length == 0) {
        return {};
    }
    
    // For very small data, compression overhead might not be worth it
    if (length < 100) {
        std::vector<uint8_t> result(data, data + length);
        return result;
    }
    
#ifdef LYRA_ZSTD_AVAILABLE
    try {
        // Get maximum compressed size
        size_t max_compressed = ZSTD_compressBound(length);
        std::vector<uint8_t> compressed(max_compressed);
        
        // Compress the data
        size_t compressed_size = ZSTD_compress(
            compressed.data(), 
            compressed.size(),
            data, 
            length,
            level_
        );
        
        // Check for errors
        if (ZSTD_isError(compressed_size)) {
            throw std::runtime_error(
                std::string("ZSTD compression failed: ") + 
                ZSTD_getErrorName(compressed_size)
            );
        }
        
        // Resize to actual compressed size
        compressed.resize(compressed_size);
        
        // Only return compressed if it's actually smaller
        if (compressed_size < length) {
            return compressed;
        } else {
            // Not worth compressing - return original
            return std::vector<uint8_t>(data, data + length);
        }
    } catch (const std::exception& e) {
        // If compression fails, return uncompressed data
        return std::vector<uint8_t>(data, data + length);
    }
#else
    // ZSTD not available - return uncompressed
    return std::vector<uint8_t>(data, data + length);
#endif
}

std::vector<uint8_t> ZstdCompressor::decompress(
    const uint8_t* data, 
    size_t length) {
    
    if (!data || length == 0) {
        return {};
    }
    
#ifdef LYRA_ZSTD_AVAILABLE
    try {
        // Get decompressed size from frame header
        unsigned long long decompressed_size = ZSTD_getFrameContentSize(data, length);
        
        if (ZSTD_isError(decompressed_size)) {
            throw std::runtime_error(
                std::string("ZSTD frame analysis failed: ") + 
                ZSTD_getErrorName(decompressed_size)
            );
        }
        
        if (decompressed_size > 1000000000) {  // 1GB limit
            throw std::runtime_error("Decompressed size exceeds 1GB limit");
        }
        
        // Allocate output buffer
        std::vector<uint8_t> decompressed(decompressed_size);
        
        // Decompress the data
        size_t result = ZSTD_decompress(
            decompressed.data(),
            decompressed.size(),
            data,
            length
        );
        
        // Check for errors
        if (ZSTD_isError(result)) {
            throw std::runtime_error(
                std::string("ZSTD decompression failed: ") + 
                ZSTD_getErrorName(result)
            );
        }
        
        return decompressed;
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("ZSTD decompression error: ") + e.what()
        );
    }
#else
    // ZSTD not available - assume data is uncompressed
    return std::vector<uint8_t>(data, data + length);
#endif
}

double ZstdCompressor::estimate_ratio(
    const uint8_t* data, 
    size_t length) {
    
    if (!data || length == 0) {
        return 1.0;
    }
    
#ifdef LYRA_ZSTD_AVAILABLE
    // Sample first 64KB if data is larger
    size_t sample_size = std::min(size_t(65536), length);
    
    try {
        // Try compressing the sample
        size_t max_compressed = ZSTD_compressBound(sample_size);
        std::vector<uint8_t> compressed(max_compressed);
        
        size_t compressed_size = ZSTD_compress(
            compressed.data(),
            compressed.size(),
            data,
            sample_size,
            3  // Use default level for estimation
        );
        
        if (ZSTD_isError(compressed_size)) {
            return 1.0;  // Estimation failed, return no benefit
        }
        
        // Calculate ratio: compressed_size / original_size
        double ratio = static_cast<double>(compressed_size) / static_cast<double>(sample_size);
        
        // Extrapolate to full data if we sampled
        if (sample_size < length) {
            // Add overhead estimate for frame header (~18 bytes)
            // but scale down for large files
            double overhead_factor = 1.0 + (18.0 / length);
            ratio = ratio * overhead_factor;
        }
        
        // Clamp ratio between 0.01 (99% compression) and 1.5 (50% expansion)
        ratio = std::max(0.01, std::min(1.5, ratio));
        
        return ratio;
    } catch (...) {
        return 1.0;  // Estimation failed
    }
#else
    // ZSTD not available - assume 1:1 ratio (no compression)
    return 1.0;
#endif
}

} // namespace compression
} // namespace lyradb
