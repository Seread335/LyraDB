#pragma once

#include <vector>
#include <cstdint>

namespace lyradb {
namespace indexes {

/**
 * @brief Bloom Filter Index
 * Fast membership testing for filtering
 */
class BloomFilter {
public:
    explicit BloomFilter(size_t size_bytes);
    
    void add(const void* data, size_t length);
    bool might_exist(const void* data, size_t length) const;
    
private:
    std::vector<uint8_t> bits_;
    
    uint32_t hash(const void* data, size_t length, uint32_t seed) const;
};

} // namespace indexes
} // namespace lyradb
