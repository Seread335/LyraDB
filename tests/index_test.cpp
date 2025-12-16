#include <gtest/gtest.h>
#include <memory>
#include "lyradb/b_tree_index.h"
#include "lyradb/hash_index.h"
#include "lyradb/bitmap_index.h"

namespace lyradb {
namespace tests {

// ============================================================================
// B-Tree Index Tests
// ============================================================================

class BTreeIndexTest : public ::testing::Test {
protected:
    lyradb::index::BTreeIndex<int, int64_t> index;
};

TEST_F(BTreeIndexTest, InsertAndSearch) {
    index.insert(10, 100);
    index.insert(20, 200);
    index.insert(15, 150);
    
    EXPECT_EQ(index.search(10).size(), 1);
    EXPECT_EQ(index.search(20).size(), 1);
    EXPECT_EQ(index.search(15).size(), 1);
    EXPECT_EQ(index.search(999).size(), 0);
}

TEST_F(BTreeIndexTest, MultipleValuesPerKey) {
    index.insert(10, 100);
    index.insert(10, 101);
    index.insert(10, 102);
    
    auto results = index.search(10);
    EXPECT_EQ(results.size(), 3);
}

TEST_F(BTreeIndexTest, Contains) {
    index.insert(42, 420);
    
    EXPECT_TRUE(index.contains(42));
    EXPECT_FALSE(index.contains(99));
}

TEST_F(BTreeIndexTest, RangeQuery) {
    for (int i = 0; i < 100; i++) {
        index.insert(i, i * 100);
    }
    
    auto results = index.range_query(20, 30);
    EXPECT_GE(results.size(), 10);  // At least 20-30
}

TEST_F(BTreeIndexTest, LessThanQuery) {
    for (int i = 0; i < 100; i++) {
        index.insert(i, i * 100);
    }
    
    auto results = index.get_less_than(50);
    EXPECT_GE(results.size(), 48);  // 0-49
}

TEST_F(BTreeIndexTest, GreaterThanQuery) {
    for (int i = 0; i < 100; i++) {
        index.insert(i, i * 100);
    }
    
    auto results = index.get_greater_than(50);
    EXPECT_GE(results.size(), 48);  // 51-99
}

TEST_F(BTreeIndexTest, DeleteEntry) {
    index.insert(10, 100);
    index.insert(10, 101);
    index.insert(20, 200);
    
    EXPECT_TRUE(index.delete_entry(10, 100));
    auto results = index.search(10);
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0], 101);
}

TEST_F(BTreeIndexTest, Size) {
    EXPECT_EQ(index.size(), 0);
    
    index.insert(1, 10);
    EXPECT_EQ(index.size(), 1);
    
    index.insert(2, 20);
    EXPECT_EQ(index.size(), 2);
}

TEST_F(BTreeIndexTest, Empty) {
    EXPECT_TRUE(index.empty());
    
    index.insert(1, 10);
    EXPECT_FALSE(index.empty());
}

TEST_F(BTreeIndexTest, Clear) {
    index.insert(1, 10);
    index.insert(2, 20);
    
    index.clear();
    EXPECT_TRUE(index.empty());
    EXPECT_EQ(index.size(), 0);
}

TEST_F(BTreeIndexTest, Height) {
    index.insert(1, 10);
    EXPECT_GE(index.height(), 1);
}

TEST_F(BTreeIndexTest, LargeInsert) {
    for (int i = 0; i < 1000; i++) {
        index.insert(i, i * 100);
    }
    
    EXPECT_EQ(index.size(), 1000);
    EXPECT_TRUE(index.contains(500));
}

// ============================================================================
// Hash Index Tests
// ============================================================================

class HashIndexTest : public ::testing::Test {
protected:
    lyradb::index::HashIndex<int, int64_t> index;
};

TEST_F(HashIndexTest, InsertAndSearch) {
    index.insert(10, 100);
    index.insert(20, 200);
    index.insert(15, 150);
    
    auto r10 = index.search(10);
    EXPECT_EQ(r10.size(), 1);
    EXPECT_EQ(r10[0], 100);
}

TEST_F(HashIndexTest, MultipleValuesPerKey) {
    index.insert(10, 100);
    index.insert(10, 101);
    index.insert(10, 102);
    
    auto results = index.search(10);
    EXPECT_EQ(results.size(), 3);
}

TEST_F(HashIndexTest, Contains) {
    index.insert(42, 420);
    
    EXPECT_TRUE(index.contains(42));
    EXPECT_FALSE(index.contains(99));
}

TEST_F(HashIndexTest, DeleteEntry) {
    index.insert(10, 100);
    index.insert(10, 101);
    
    EXPECT_TRUE(index.delete_entry(10, 100));
    auto results = index.search(10);
    EXPECT_EQ(results.size(), 1);
}

TEST_F(HashIndexTest, DeleteAllValues) {
    index.insert(10, 100);
    
    index.delete_entry(10, 100);
    EXPECT_FALSE(index.contains(10));
}

TEST_F(HashIndexTest, GetAll) {
    index.insert(10, 100);
    index.insert(20, 200);
    index.insert(30, 300);
    
    auto all = index.get_all();
    EXPECT_EQ(all.size(), 3);
}

TEST_F(HashIndexTest, Size) {
    EXPECT_EQ(index.size(), 0);
    
    index.insert(1, 10);
    EXPECT_EQ(index.size(), 1);
}

TEST_F(HashIndexTest, LoadFactor) {
    index.insert(1, 10);
    double lf = index.load_factor();
    
    EXPECT_LT(lf, 1.0);
    EXPECT_GT(lf, 0.0);
}

TEST_F(HashIndexTest, Resize) {
    for (int i = 0; i < 1000; i++) {
        index.insert(i, i * 100);
    }
    
    EXPECT_EQ(index.size(), 1000);
    EXPECT_LT(index.load_factor(), 0.75);
}

TEST_F(HashIndexTest, Clear) {
    index.insert(1, 10);
    index.insert(2, 20);
    
    index.clear();
    EXPECT_TRUE(index.empty());
}

// ============================================================================
// Bitmap Index Tests
// ============================================================================

class BitmapIndexTest : public ::testing::Test {
protected:
    lyradb::index::BitmapIndex<int> index;
};

TEST_F(BitmapIndexTest, InsertAndSearch) {
    index.insert(10, 100);
    index.insert(10, 101);
    index.insert(20, 200);
    
    auto r10 = index.search(10);
    EXPECT_EQ(r10.size(), 2);
}

TEST_F(BitmapIndexTest, Contains) {
    index.insert(42, 420);
    
    EXPECT_TRUE(index.contains(42));
    EXPECT_FALSE(index.contains(99));
}

TEST_F(BitmapIndexTest, GetAnyOf) {
    index.insert(10, 100);
    index.insert(20, 200);
    index.insert(30, 300);
    
    std::vector<int> keys = {10, 20};
    auto results = index.get_any_of(keys);
    EXPECT_EQ(results.size(), 2);
}

TEST_F(BitmapIndexTest, GetAllOf) {
    index.insert(10, 100);
    index.insert(10, 101);
    index.insert(20, 100);  // Shared row
    
    std::vector<int> keys = {10, 20};
    auto results = index.get_all_of(keys);
    EXPECT_EQ(results.size(), 1);  // Only row 100
}

TEST_F(BitmapIndexTest, GetNot) {
    index.insert(10, 100);
    index.insert(10, 101);
    index.insert(20, 102);
    
    auto results = index.get_not(10);
    EXPECT_EQ(results.size(), 1);  // Row 102
}

TEST_F(BitmapIndexTest, GetDistinctKeys) {
    index.insert(10, 100);
    index.insert(20, 200);
    index.insert(30, 300);
    
    auto keys = index.get_distinct_keys();
    EXPECT_EQ(keys.size(), 3);
}

TEST_F(BitmapIndexTest, DeleteKey) {
    index.insert(10, 100);
    index.insert(10, 101);
    
    size_t deleted = index.delete_key(10);
    EXPECT_EQ(deleted, 2);
    EXPECT_FALSE(index.contains(10));
}

TEST_F(BitmapIndexTest, Size) {
    EXPECT_EQ(index.size(), 0);
    
    index.insert(10, 100);
    EXPECT_EQ(index.size(), 1);
    
    index.insert(20, 200);
    EXPECT_EQ(index.size(), 2);
}

TEST_F(BitmapIndexTest, Cardinality) {
    index.insert(10, 100);
    index.insert(10, 101);
    index.insert(20, 200);
    
    EXPECT_EQ(index.cardinality(), 2);  // 2 distinct keys
}

TEST_F(BitmapIndexTest, MemoryUsage) {
    index.insert(10, 100);
    size_t mem = index.memory_usage();
    
    EXPECT_GT(mem, 0);
}

TEST_F(BitmapIndexTest, Clear) {
    index.insert(10, 100);
    index.insert(20, 200);
    
    index.clear();
    EXPECT_TRUE(index.empty());
}

// ============================================================================
// Index Comparison Tests
// ============================================================================

TEST(IndexComparison, BTreeRangeQuery) {
    lyradb::index::BTreeIndex<int, int64_t> idx;
    
    for (int i = 0; i < 1000; i++) {
        idx.insert(i, i * 10);
    }
    
    auto results = idx.range_query(100, 200);
    EXPECT_GE(results.size(), 100);  // 100-200
}

TEST(IndexComparison, HashIndexEquality) {
    lyradb::index::HashIndex<std::string, int64_t> idx;
    
    idx.insert("alice", 10);
    idx.insert("bob", 20);
    idx.insert("charlie", 30);
    
    EXPECT_TRUE(idx.contains("alice"));
    EXPECT_EQ(idx.search("bob").size(), 1);
}

TEST(IndexComparison, BitmapIndexBitwiseOps) {
    lyradb::index::BitmapIndex<std::string> idx;
    
    idx.insert("active", 0);
    idx.insert("active", 1);
    idx.insert("inactive", 2);
    idx.insert("inactive", 3);
    
    std::vector<std::string> keys = {"active", "inactive"};
    auto all = idx.get_any_of(keys);
    EXPECT_EQ(all.size(), 4);
}

} // namespace tests
} // namespace lyradb
