#include "lyradb/dict_compressor.h"
#include <algorithm>
#include <cstring>

namespace lyradb {
namespace compression {

std::vector<uint8_t> DictionaryCompressor::compress(
    const std::vector<std::string>& values) {
    
    if (values.empty()) {
        return {};
    }
    
    // Build dictionary
    auto dict = build_dictionary(values);
    
    std::vector<uint8_t> result;
    
    // Write dictionary header: number of entries
    uint32_t dict_size = dict.size();
    result.push_back(dict_size & 0xFF);
    result.push_back((dict_size >> 8) & 0xFF);
    result.push_back((dict_size >> 16) & 0xFF);
    result.push_back((dict_size >> 24) & 0xFF);
    
    // Write dictionary entries
    for (const auto& entry : dict) {
        // Key length (2 bytes)
        uint16_t key_len = entry.key.length();
        result.push_back(key_len & 0xFF);
        result.push_back((key_len >> 8) & 0xFF);
        
        // Key bytes
        result.insert(result.end(), entry.key.begin(), entry.key.end());
        
        // Value ID (4 bytes)
        uint32_t id = entry.id;
        result.push_back(id & 0xFF);
        result.push_back((id >> 8) & 0xFF);
        result.push_back((id >> 16) & 0xFF);
        result.push_back((id >> 24) & 0xFF);
    }
    
    // Write compressed values (IDs only)
    for (const auto& value : values) {
        // Find ID for this value
        uint32_t id = 0;
        for (const auto& entry : dict) {
            if (entry.key == value) {
                id = entry.id;
                break;
            }
        }
        
        result.push_back(id & 0xFF);
        result.push_back((id >> 8) & 0xFF);
        result.push_back((id >> 16) & 0xFF);
        result.push_back((id >> 24) & 0xFF);
    }
    
    return result;
}

std::vector<std::string> DictionaryCompressor::decompress(
    const uint8_t* data, 
    size_t length) {
    
    if (!data || length < 4) {
        return {};
    }
    
    std::vector<std::string> result;
    size_t pos = 0;
    
    // Read dictionary size
    uint32_t dict_size = 
        data[pos] | 
        (data[pos + 1] << 8) | 
        (data[pos + 2] << 16) | 
        (data[pos + 3] << 24);
    pos += 4;
    
    // Read dictionary
    std::vector<std::string> dict(dict_size);
    for (uint32_t i = 0; i < dict_size; ++i) {
        if (pos + 2 > length) return result;
        
        uint16_t key_len = data[pos] | (data[pos + 1] << 8);
        pos += 2;
        
        if (pos + key_len > length) return result;
        
        dict[i] = std::string(
            reinterpret_cast<const char*>(data + pos), 
            key_len);
        pos += key_len;
        
        // Skip ID (4 bytes)
        pos += 4;
    }
    
    // Read and decompress values
    while (pos + 4 <= length) {
        uint32_t id = 
            data[pos] | 
            (data[pos + 1] << 8) | 
            (data[pos + 2] << 16) | 
            (data[pos + 3] << 24);
        pos += 4;
        
        if (id < dict.size()) {
            result.push_back(dict[id]);
        }
    }
    
    return result;
}

double DictionaryCompressor::estimate_compression_ratio(
    const std::vector<std::string>& values) {
    
    if (values.empty()) {
        return 1.0;
    }
    
    // Count unique values
    std::unordered_map<std::string, uint32_t> freq;
    for (const auto& val : values) {
        freq[val]++;
    }
    
    // Original size: sum of all string lengths
    size_t original_size = 0;
    for (const auto& val : values) {
        original_size += val.length();
    }
    
    // Compressed size estimate:
    // - Dictionary header: 4 bytes
    // - Dict entries: sum of (2 + key_len + 4) for each unique value
    // - Values: 4 bytes * num_values (IDs only)
    
    size_t dict_size = 4;  // Header
    for (const auto& [key, _] : freq) {
        dict_size += 2 + key.length() + 4;  // len + key + id
    }
    
    size_t value_size = values.size() * 4;  // IDs
    size_t total_compressed = dict_size + value_size;
    
    return static_cast<double>(total_compressed) / original_size;
}

bool DictionaryCompressor::is_suitable(
    const std::vector<std::string>& values,
    double cardinality_threshold) {
    
    if (values.empty()) {
        return false;
    }
    
    // Count unique values
    std::unordered_map<std::string, uint32_t> freq;
    for (const auto& val : values) {
        freq[val]++;
    }
    
    double cardinality = static_cast<double>(freq.size()) / values.size();
    return cardinality < cardinality_threshold;
}

std::vector<DictionaryCompressor::DictEntry> 
DictionaryCompressor::build_dictionary(
    const std::vector<std::string>& values) {
    
    std::unordered_map<std::string, uint32_t> freq;
    for (const auto& val : values) {
        freq[val]++;
    }
    
    std::vector<DictEntry> dict;
    uint32_t id = 0;
    
    for (auto& [key, frequency] : freq) {
        dict.push_back({key, id++, frequency});
    }
    
    // Sort by frequency (descending) for better compression
    std::sort(dict.begin(), dict.end(),
        [](const DictEntry& a, const DictEntry& b) {
            return a.frequency > b.frequency;
        });
    
    // Update IDs after sorting
    for (uint32_t i = 0; i < dict.size(); ++i) {
        dict[i].id = i;
    }
    
    return dict;
}

} // namespace compression
} // namespace lyradb
