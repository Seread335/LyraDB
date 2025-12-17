#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <map>
#include <set>
#include "lru2.h"

namespace lyradb {

using PageId = uint32_t;

/**
 * @brief Page handle for safe page access
 */
class PageHandle {
public:
    explicit PageHandle(void* data) : data_(data) {}
    
    void* data() const { return data_; }
    
private:
    void* data_;
};

/**
 * @brief Buffer Manager with LRU2 replacement policy
 * Manages in-memory page buffer for columnar data
 */
class BufferManager {
public:
    /**
     * @brief Initialize buffer pool
     * @param pool_size Total size in bytes
     * @param page_size Default page size (default 64KB)
     */
    BufferManager(size_t pool_size, size_t page_size = 65536);
    
    /**
     * @brief Get page from buffer (loads from disk if needed)
     */
    PageHandle get_page(PageId page_id);
    
    /**
     * @brief Pin a page in memory (prevent eviction)
     */
    void pin_page(PageId page_id);
    
    /**
     * @brief Unpin a page
     */
    void unpin_page(PageId page_id);
    
    /**
     * @brief Check if page is pinned
     */
    bool is_pinned(PageId page_id) const;
    
    /**
     * @brief Mark page as modified
     */
    void mark_dirty(PageId page_id);
    
    /**
     * @brief Write all dirty pages to disk
     */
    void flush_all();
    
    size_t pool_size() const { return pool_size_; }
    size_t page_size() const { return page_size_; }
    size_t num_pages() const { return page_map_.size(); }
    
private:
    size_t pool_size_;
    size_t page_size_;
    std::vector<uint8_t> buffer_pool_;
    std::map<PageId, void*> page_map_;
    std::map<PageId, uint32_t> pin_count_;
    std::set<PageId> dirty_pages_;
    LRU2<PageId> lru2_;
};

} // namespace lyradb
