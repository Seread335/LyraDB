#pragma once

namespace lyradb {
namespace indexes {

/**
 * @brief Zone Map Index
 * Min/max values per page for range filtering
 */
class ZoneMapIndex {
public:
    ZoneMapIndex() = default;
    
    // TODO: Implement zone map functionality
    // - Track min/max per page
    // - Enable range predicate pushdown
};

} // namespace indexes
} // namespace lyradb
