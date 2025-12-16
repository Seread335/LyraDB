#include "lyradb/buffer_manager.h"
#include <stdexcept>

namespace lyradb {

BufferManager::BufferManager(size_t pool_size, size_t page_size)
    : pool_size_(pool_size), page_size_(page_size), lru2_(pool_size) {
    buffer_pool_.resize(pool_size);
}

PageHandle BufferManager::get_page(PageId page_id) {
    auto it = page_map_.find(page_id);
    if (it != page_map_.end()) {
        lru2_.access(page_id);
        return PageHandle(it->second);
    }
    
    // Page not in memory, need to load from disk
    // TODO: Implement disk I/O
    throw std::runtime_error("Page not in buffer");
}

void BufferManager::pin_page(PageId page_id) {
    auto it = pin_count_.find(page_id);
    if (it != pin_count_.end()) {
        it->second++;
    } else {
        pin_count_[page_id] = 1;
    }
}

void BufferManager::unpin_page(PageId page_id) {
    auto it = pin_count_.find(page_id);
    if (it != pin_count_.end() && it->second > 0) {
        it->second--;
        if (it->second == 0) {
            pin_count_.erase(it);
        }
    }
}

bool BufferManager::is_pinned(PageId page_id) const {
    auto it = pin_count_.find(page_id);
    return it != pin_count_.end() && it->second > 0;
}

void BufferManager::mark_dirty(PageId page_id) {
    dirty_pages_.insert(page_id);
}

void BufferManager::flush_all() {
    // TODO: Write all dirty pages to disk
}

} // namespace lyradb
