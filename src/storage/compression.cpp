#include "lyradb/compression.h"
#include "lyradb/zstd_compressor.h"
#include "lyradb/rle_compressor.h"
#include "lyradb/dict_compressor.h"
#include <stdexcept>

namespace lyradb {

std::vector<uint8_t> Compression::compress(
    const std::vector<uint8_t>& data,
    CompressionType type) {
    
    switch (type) {
        case CompressionType::NONE:
            return data;
        case CompressionType::ZSTD:
            return compress_zstd(data);
        case CompressionType::RLE:
            return compress_rle(data);
        case CompressionType::DICTIONARY:
            return compress_dictionary(data);
        default:
            throw std::runtime_error("Unknown compression type");
    }
}

std::vector<uint8_t> Compression::decompress(
    const std::vector<uint8_t>& data,
    CompressionType type) {
    
    switch (type) {
        case CompressionType::NONE:
            return data;
        case CompressionType::ZSTD:
            return decompress_zstd(data);
        case CompressionType::RLE:
            return decompress_rle(data);
        case CompressionType::DICTIONARY:
            return decompress_dictionary(data);
        default:
            throw std::runtime_error("Unknown compression type");
    }
}

std::vector<uint8_t> Compression::compress_zstd(const std::vector<uint8_t>& data) {
    try {
        compression::ZstdCompressor compressor(3);  // Default level
        return compressor.compress(data.data(), data.size());
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("ZSTD compression failed: ") + e.what());
    }
}

std::vector<uint8_t> Compression::decompress_zstd(const std::vector<uint8_t>& data) {
    try {
        return compression::ZstdCompressor::decompress(data.data(), data.size());
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("ZSTD decompression failed: ") + e.what());
    }
}

std::vector<uint8_t> Compression::compress_rle(const std::vector<uint8_t>& data) {
    try {
        // RLE works best on columnar data with value_size known
        // For generic data, assume value_size of 8 (int64_t or double)
        size_t value_size = 8;
        
        // If data is too small, no compression benefit
        if (data.size() < 16) {
            return data;
        }
        
        auto compressed = compression::RLECompressor::compress(
            data.data(), data.size(), value_size);
        
        // Only return compressed if beneficial
        if (compressed.size() >= data.size()) {
            return data;
        }
        
        return compressed;
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("RLE compression failed: ") + e.what());
    }
}

std::vector<uint8_t> Compression::decompress_rle(const std::vector<uint8_t>& data) {
    try {
        if (data.empty()) {
            return data;
        }
        
        // RLE decompressor with value_size of 8
        size_t value_size = 8;
        return compression::RLECompressor::decompress(
            data.data(), data.size(), value_size);
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("RLE decompression failed: ") + e.what());
    }
}

std::vector<uint8_t> Compression::compress_dictionary(const std::vector<uint8_t>& data) {
    try {
        // Dictionary compression works on string data
        // For now, treat binary data as strings
        std::vector<std::string> strings;
        
        // Convert binary data to strings (split by null terminators)
        size_t start = 0;
        for (size_t i = 0; i < data.size(); ++i) {
            if (data[i] == '\0' || i == data.size() - 1) {
                if (i > start || (i == data.size() - 1 && data[i] != '\0')) {
                    if (i == data.size() - 1 && data[i] != '\0') {
                        strings.push_back(std::string(
                            reinterpret_cast<const char*>(data.data() + start),
                            i - start + 1));
                    } else {
                        strings.push_back(std::string(
                            reinterpret_cast<const char*>(data.data() + start),
                            i - start));
                    }
                }
                start = i + 1;
            }
        }
        
        if (strings.empty()) {
            return data;
        }
        
        // Check if dictionary compression is suitable
        if (!compression::DictionaryCompressor::is_suitable(strings)) {
            return data;
        }
        
        return compression::DictionaryCompressor::compress(strings);
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Dictionary compression failed: ") + e.what());
    }
}

std::vector<uint8_t> Compression::decompress_dictionary(const std::vector<uint8_t>& data) {
    try {
        if (data.empty()) {
            return data;
        }
        
        auto strings = compression::DictionaryCompressor::decompress(data.data(), data.size());
        
        // Convert strings back to binary
        std::vector<uint8_t> result;
        for (const auto& str : strings) {
            result.insert(result.end(), str.begin(), str.end());
            result.push_back('\0');  // Null terminator between strings
        }
        
        return result;
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Dictionary decompression failed: ") + e.what());
    }
}

} // namespace lyradb
