#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <chrono>
#include "../include/lyradb/b_tree.h"
#include "../include/lyradb/b_tree_impl.h"
#include "../include/lyradb/composite_key.h"

using namespace lyradb::index;

// Test 1: Basic B-tree insert and search
void test_btree_basic_operations() {
    std::cout << "\n=== Test 1: Basic B-tree Insert and Search ===" << std::endl;
    
    BTree<std::string, size_t> btree;
    
    // Insert test data
    btree.insert("apple", 1);
    btree.insert("banana", 2);
    btree.insert("cherry", 3);
    btree.insert("date", 4);
    btree.insert("elderberry", 5);
    
    // Search tests
    auto result1 = btree.search("apple");
    assert(result1.size() == 1 && result1[0] == 1);
    std::cout << "✓ Found 'apple' with value 1" << std::endl;
    
    auto result2 = btree.search("cherry");
    assert(result2.size() == 1 && result2[0] == 3);
    std::cout << "✓ Found 'cherry' with value 3" << std::endl;
    
    auto result3 = btree.search("unknown");
    assert(result3.empty());
    std::cout << "✓ 'unknown' not found (as expected)" << std::endl;
    
    std::cout << "✓ Basic operations test passed!" << std::endl;
}

// Test 2: B-tree range search
void test_btree_range_search() {
    std::cout << "\n=== Test 2: B-tree Range Search ===" << std::endl;
    
    BTree<std::string, size_t> btree;
    
    // Insert test data
    std::vector<std::pair<std::string, size_t>> data = {
        {"apple", 1},
        {"apricot", 2},
        {"avocado", 3},
        {"banana", 4},
        {"blueberry", 5},
        {"cherry", 6}
    };
    
    for (const auto& [key, val] : data) {
        btree.insert(key, val);
    }
    
    // Range search [apple, banana]
    auto results = btree.range_search("apple", "banana");
    assert(results.size() == 4); // apple, apricot, avocado, banana
    std::cout << "✓ Range [apple, banana] returned " << results.size() << " results" << std::endl;
    
    // Range search [apricot, blueberry]
    results = btree.range_search("apricot", "blueberry");
    assert(results.size() == 4); // apricot, avocado, banana, blueberry
    std::cout << "✓ Range [apricot, blueberry] returned " << results.size() << " results" << std::endl;
    
    std::cout << "✓ Range search test passed!" << std::endl;
}

// Test 3: Composite key with B-tree
void test_composite_key_comparison() {
    std::cout << "\n=== Test 3: Composite Key Comparison ===" << std::endl;
    
    CompositeKey key1, key2, key3;
    
    key1.add_value("alice");
    key1.add_value("100");
    
    key2.add_value("alice");
    key2.add_value("200");
    
    key3.add_value("bob");
    key3.add_value("100");
    
    // Test lexicographic ordering
    assert(key1 < key2);  // same first, different second
    std::cout << "✓ (alice,100) < (alice,200)" << std::endl;
    
    assert(key1 < key3);  // different first column
    std::cout << "✓ (alice,100) < (bob,100)" << std::endl;
    
    assert(key2 > key1);
    std::cout << "✓ (alice,200) > (alice,100)" << std::endl;
    
    assert(key1 <= key2);
    std::cout << "✓ (alice,100) <= (alice,200)" << std::endl;
    
    assert(key2 >= key1);
    std::cout << "✓ (alice,200) >= (alice,100)" << std::endl;
    
    std::cout << "✓ Composite key comparison test passed!" << std::endl;
}

// Test 4: B-tree with numeric keys
void test_btree_numeric_keys() {
    std::cout << "\n=== Test 4: B-tree with Numeric Keys ===" << std::endl;
    
    BTree<int, std::string> btree;
    
    // Insert test data (not in order to test balancing)
    btree.insert(50, "fifty");
    btree.insert(30, "thirty");
    btree.insert(70, "seventy");
    btree.insert(20, "twenty");
    btree.insert(40, "forty");
    btree.insert(60, "sixty");
    btree.insert(80, "eighty");
    
    // Verify exact matches
    auto result1 = btree.search(50);
    assert(result1.size() == 1 && result1[0] == "fifty");
    
    auto result2 = btree.search(20);
    assert(result2.size() == 1 && result2[0] == "twenty");
    
    auto result3 = btree.search(80);
    assert(result3.size() == 1 && result3[0] == "eighty");
    std::cout << "✓ All exact match searches successful" << std::endl;
    
    // Range search
    auto results = btree.range_search(30, 70);
    assert(results.size() == 5); // 30, 40, 50, 60, 70
    std::cout << "✓ Range [30, 70] returned " << results.size() << " results" << std::endl;
    
    std::cout << "✓ Numeric key test passed!" << std::endl;
}

// Test 5: Large dataset insertion
void test_btree_large_dataset() {
    std::cout << "\n=== Test 5: Large Dataset Insertion ===" << std::endl;
    
    BTree<int, size_t> btree;
    const int NUM_ELEMENTS = 1000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Insert elements
    for (int i = 0; i < NUM_ELEMENTS; ++i) {
        btree.insert(i, static_cast<size_t>(i * 2));
    }
    
    auto insert_time = std::chrono::high_resolution_clock::now();
    
    // Search for elements
    int found_count = 0;
    for (int i = 0; i < NUM_ELEMENTS; i += 100) {
        auto result = btree.search(i);
        if (result.size() == 1 && result[0] == static_cast<size_t>(i * 2)) {
            found_count++;
        }
    }
    
    auto search_time = std::chrono::high_resolution_clock::now();
    
    auto insert_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        insert_time - start
    );
    auto search_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        search_time - insert_time
    );
    
    assert(found_count == 10); // 0, 100, 200, ..., 900
    std::cout << "✓ Inserted " << NUM_ELEMENTS << " elements in " << insert_duration.count() << "ms" << std::endl;
    std::cout << "✓ Searched 10 elements in " << search_duration.count() << "ms" << std::endl;
    
    // Test range search
    auto results = btree.range_search(100, 200);
    assert(results.size() == 101); // 100 to 200 inclusive
    std::cout << "✓ Range [100, 200] returned " << results.size() << " results" << std::endl;
    
    std::cout << "✓ Large dataset test passed!" << std::endl;
}

// Test 6: B-tree balancing and tree structure
void test_btree_balance() {
    std::cout << "\n=== Test 6: B-tree Balance Verification ===" << std::endl;
    
    BTree<int, int> btree;
    
    // Insert many elements in order to force splits
    for (int i = 1; i <= 100; ++i) {
        btree.insert(i, i * 10);
    }
    
    // Verify all elements can be found
    int found = 0;
    for (int i = 1; i <= 100; ++i) {
        auto result = btree.search(i);
        if (result.size() == 1) {
            found++;
        }
    }
    
    assert(found == 100);
    std::cout << "✓ All 100 elements found after insertions with splits" << std::endl;
    
    // Test range search after splits
    auto results = btree.range_search(25, 75);
    assert(results.size() == 51); // 25 to 75 inclusive
    std::cout << "✓ Range search [25, 75] returned " << results.size() << " results" << std::endl;
    
    std::cout << "✓ Balance test passed!" << std::endl;
}

int main() {
    std::cout << "╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║         B-Tree Index Test Suite (Phase 4.2)               ║" << std::endl;
    std::cout << "║  Testing range query index with lexicographic ordering    ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝" << std::endl;
    
    try {
        test_btree_basic_operations();
        test_btree_range_search();
        test_composite_key_comparison();
        test_btree_numeric_keys();
        test_btree_large_dataset();
        test_btree_balance();
        
        std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║           ✓ All B-tree Tests Passed!                       ║" << std::endl;
        std::cout << "╚═══════════════════════════════════════════════════════════╝" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
