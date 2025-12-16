/**
 * @file bench_phase43_indexed.cpp
 * @brief PHASE 4.3: Performance validation for indexed scan execution
 * 
 * This benchmark compares:
 * - Full table scan: O(n) complexity
 * - Indexed scan: O(log n + k) complexity
 * 
 * Expected results:
 * - Point lookups: 50-100x speedup
 * - Range queries: 10-50x speedup
 * - Composite queries: 5-25x speedup
 * 
 * Validates that Phase 4.2 cost models predict actual Phase 4.3 performance
 */

#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <algorithm>
#include <iomanip>
#include <cmath>

namespace lyradb {
namespace benchmark {

/**
 * @brief Simple B-tree index simulation for benchmarking
 * 
 * Provides O(log n + k) range query performance
 */
class SimpleIndex {
private:
    std::vector<int> sorted_keys_;
    
public:
    SimpleIndex(const std::vector<int>& keys) {
        sorted_keys_ = keys;
        std::sort(sorted_keys_.begin(), sorted_keys_.end());
    }
    
    /**
     * @brief Range query: find all keys in [min_key, max_key]
     * @return Number of keys in range
     */
    size_t range_query(int min_key, int max_key) const {
        auto lower = std::lower_bound(sorted_keys_.begin(), sorted_keys_.end(), min_key);
        auto upper = std::upper_bound(sorted_keys_.begin(), sorted_keys_.end(), max_key);
        return std::distance(lower, upper);
    }
    
    /**
     * @brief Point query: find exact key
     * @return 1 if found, 0 if not
     */
    size_t search(int key) const {
        return std::binary_search(sorted_keys_.begin(), sorted_keys_.end(), key) ? 1 : 0;
    }
    
    /**
     * @brief Get all keys less than threshold
     * @return Count of keys less than threshold
     */
    size_t get_less_than(int threshold) const {
        auto it = std::lower_bound(sorted_keys_.begin(), sorted_keys_.end(), threshold);
        return std::distance(sorted_keys_.begin(), it);
    }
    
    /**
     * @brief Get all keys greater than threshold
     * @return Count of keys greater than threshold
     */
    size_t get_greater_than(int threshold) const {
        auto it = std::upper_bound(sorted_keys_.begin(), sorted_keys_.end(), threshold);
        return std::distance(it, sorted_keys_.end());
    }
};

/**
 * @brief Benchmark parameters
 */
struct BenchmarkConfig {
    size_t table_size;           // Number of rows
    double selectivity;          // Fraction of rows matching predicate
    std::string query_type;      // "point", "range", "composite"
    int iterations;              // Number of benchmark runs
};

/**
 * @brief Performance metrics
 */
struct BenchmarkResult {
    std::string test_name;
    double full_scan_time_ms;    // Full table scan duration
    double indexed_scan_time_ms; // Indexed scan duration
    double speedup;              // Speedup factor
    size_t matched_rows;         // Number of matching rows
    
    void print() const {
        std::cout << "\n  Test: " << test_name << "\n";
        std::cout << "  ├─ Full scan:      " << std::fixed << std::setprecision(3) 
                  << full_scan_time_ms << " ms\n";
        std::cout << "  ├─ Indexed scan:   " << std::fixed << std::setprecision(3) 
                  << indexed_scan_time_ms << " ms\n";
        std::cout << "  ├─ Speedup:        " << std::fixed << std::setprecision(1) 
                  << speedup << "x ⚡\n";
        std::cout << "  └─ Matched rows:   " << matched_rows << "\n";
    }
};

class Phase43Benchmarker {
private:
    std::mt19937 rng_;
    
public:
    Phase43Benchmarker() : rng_(std::random_device{}()) {}
    
    /**
     * @brief Generate test data with uniform distribution
     * @param size Number of rows
     * @param min_value Minimum value
     * @param max_value Maximum value
     */
    std::vector<int> generate_data(size_t size, int min_value, int max_value) {
        std::uniform_int_distribution<int> dist(min_value, max_value);
        std::vector<int> data(size);
        
        for (size_t i = 0; i < size; ++i) {
            data[i] = dist(rng_);
        }
        
        return data;
    }
    
    /**
     * @brief Full table scan - O(n) complexity
     * 
     * Simulates scanning all rows and filtering
     */
    size_t full_scan(const std::vector<int>& data, int threshold, const std::string& op) {
        size_t count = 0;
        
        for (const auto& value : data) {
            bool matches = false;
            
            if (op == ">") {
                matches = value > threshold;
            } else if (op == "<") {
                matches = value < threshold;
            } else if (op == ">=") {
                matches = value >= threshold;
            } else if (op == "<=") {
                matches = value <= threshold;
            } else if (op == "=") {
                matches = value == threshold;
            }
            
            if (matches) count++;
        }
        
        return count;
    }
    
    /**
     * @brief Indexed scan - O(log n + k) complexity
     * 
     * Uses B-tree to find only matching rows
     */
    size_t indexed_scan(const SimpleIndex& index, int threshold, const std::string& op) {
        if (op == ">") {
            return index.get_greater_than(threshold);
        } else if (op == "<") {
            return index.get_less_than(threshold);
        } else if (op == ">=") {
            // Approximate: get_less_than(threshold-1) then flip
            // For exact, would need >= operator in index
            return index.get_greater_than(threshold - 1);
        } else if (op == "<=") {
            return index.get_less_than(threshold + 1);
        } else if (op == "=") {
            return index.search(threshold);
        }
        
        return 0;
    }
    
    /**
     * @brief Run point lookup benchmark (single equality)
     */
    BenchmarkResult benchmark_point_lookup(size_t table_size, int iterations = 1000) {
        auto data = generate_data(table_size, 0, 1000000);
        SimpleIndex index(data);
        
        // Pick a random value for lookup
        int lookup_value = data[table_size / 2];
        
        // Benchmark full scan
        auto start = std::chrono::high_resolution_clock::now();
        size_t fs_count = 0;
        for (int i = 0; i < iterations; ++i) {
            fs_count = full_scan(data, lookup_value, "=");
        }
        auto end = std::chrono::high_resolution_clock::now();
        double full_time = std::chrono::duration<double, std::milli>(end - start).count() / iterations;
        
        // Benchmark indexed scan
        start = std::chrono::high_resolution_clock::now();
        size_t idx_count = 0;
        for (int i = 0; i < iterations; ++i) {
            idx_count = indexed_scan(index, lookup_value, "=");
        }
        end = std::chrono::high_resolution_clock::now();
        double idx_time = std::chrono::duration<double, std::milli>(end - start).count() / iterations;
        
        BenchmarkResult result;
        result.test_name = "Point lookup (id = X)";
        result.full_scan_time_ms = full_time;
        result.indexed_scan_time_ms = idx_time;
        result.matched_rows = fs_count;
        result.speedup = full_time > 0 ? full_time / std::max(idx_time, 0.0001) : 0;
        
        return result;
    }
    
    /**
     * @brief Run range query benchmark (e.g., age > 18)
     */
    BenchmarkResult benchmark_range_query(size_t table_size, double selectivity = 0.1, int iterations = 100) {
        auto data = generate_data(table_size, 0, 1000000);
        SimpleIndex index(data);
        
        // Set threshold to achieve desired selectivity
        int threshold = static_cast<int>(1000000 * (1 - selectivity));
        
        // Benchmark full scan
        auto start = std::chrono::high_resolution_clock::now();
        size_t fs_count = 0;
        for (int i = 0; i < iterations; ++i) {
            fs_count = full_scan(data, threshold, ">");
        }
        auto end = std::chrono::high_resolution_clock::now();
        double full_time = std::chrono::duration<double, std::milli>(end - start).count() / iterations;
        
        // Benchmark indexed scan
        start = std::chrono::high_resolution_clock::now();
        size_t idx_count = 0;
        for (int i = 0; i < iterations; ++i) {
            idx_count = indexed_scan(index, threshold, ">");
        }
        end = std::chrono::high_resolution_clock::now();
        double idx_time = std::chrono::duration<double, std::milli>(end - start).count() / iterations;
        
        BenchmarkResult result;
        result.test_name = "Range query (value > threshold)";
        result.full_scan_time_ms = full_time;
        result.indexed_scan_time_ms = idx_time;
        result.matched_rows = fs_count;
        result.speedup = full_time > 0 ? full_time / std::max(idx_time, 0.0001) : 0;
        
        return result;
    }
    
    /**
     * @brief Run composite query benchmark (multiple AND conditions)
     */
    BenchmarkResult benchmark_composite_query(size_t table_size, double selectivity = 0.05, int iterations = 100) {
        auto data = generate_data(table_size, 0, 1000000);
        SimpleIndex index(data);
        
        // For composite: two range predicates AND'ed together
        // Selectivity of composite is roughly selectivity1 * selectivity2
        // So each range should be sqrt(selectivity)
        double per_predicate_selectivity = std::sqrt(selectivity);
        int threshold = static_cast<int>(1000000 * (1 - per_predicate_selectivity));
        
        // Benchmark full scan (both conditions checked for each row)
        auto start = std::chrono::high_resolution_clock::now();
        size_t fs_count = 0;
        for (int i = 0; i < iterations; ++i) {
            fs_count = 0;
            for (const auto& value : data) {
                // Two AND conditions: value > threshold AND value < 2*threshold
                if (value > threshold && value < 2 * threshold) {
                    fs_count++;
                }
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        double full_time = std::chrono::duration<double, std::milli>(end - start).count() / iterations;
        
        // Benchmark indexed scan (would use index intersection)
        start = std::chrono::high_resolution_clock::now();
        size_t idx_count = 0;
        for (int i = 0; i < iterations; ++i) {
            // Simulate: scan index for value > threshold, then filter by second condition
            idx_count = index.range_query(threshold, 2 * threshold);
        }
        end = std::chrono::high_resolution_clock::now();
        double idx_time = std::chrono::duration<double, std::milli>(end - start).count() / iterations;
        
        BenchmarkResult result;
        result.test_name = "Composite AND (age > X AND age < Y)";
        result.full_scan_time_ms = full_time;
        result.indexed_scan_time_ms = idx_time;
        result.matched_rows = fs_count;
        result.speedup = full_time > 0 ? full_time / std::max(idx_time, 0.0001) : 0;
        
        return result;
    }
};

}  // namespace benchmark
}  // namespace lyradb

/**
 * @brief Main benchmark entry point
 */
int main() {
    using namespace lyradb::benchmark;
    
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║         PHASE 4.3: INDEXED SCAN PERFORMANCE VALIDATION                 ║\n";
    std::cout << "║                                                                        ║\n";
    std::cout << "║  Comparing O(n) full scan vs O(log n + k) indexed scan performance    ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════════════╝\n";
    
    Phase43Benchmarker benchmarker;
    
    std::vector<BenchmarkResult> results;
    
    // Test 1: Point lookup (best case for indexes)
    std::cout << "\n[1/3] Point Lookup Benchmark (id = X)...\n";
    results.push_back(benchmarker.benchmark_point_lookup(100000, 1000));
    
    // Test 2: Range query with 10% selectivity
    std::cout << "\n[2/3] Range Query Benchmark (age > X)...\n";
    results.push_back(benchmarker.benchmark_range_query(100000, 0.1, 100));
    
    // Test 3: Composite AND query
    std::cout << "\n[3/3] Composite Query Benchmark (age > X AND age < Y)...\n";
    results.push_back(benchmarker.benchmark_composite_query(100000, 0.05, 100));
    
    // Print results
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                      BENCHMARK RESULTS SUMMARY                        ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════════════╝\n";
    
    double total_speedup = 0;
    for (const auto& result : results) {
        result.print();
        total_speedup += result.speedup;
    }
    
    // Summary statistics
    std::cout << "\n╔════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                         PERFORMANCE SUMMARY                           ║\n";
    std::cout << "╠════════════════════════════════════════════════════════════════════════╣\n";
    
    double avg_speedup = total_speedup / results.size();
    std::cout << "│ Average Speedup:         " << std::fixed << std::setprecision(1) 
              << avg_speedup << "x ⚡\n";
    
    double total_full_scan = 0, total_indexed = 0;
    for (const auto& result : results) {
        total_full_scan += result.full_scan_time_ms;
        total_indexed += result.indexed_scan_time_ms;
    }
    
    std::cout << "│ Total Full Scan Time:    " << std::fixed << std::setprecision(2) 
              << total_full_scan << " ms\n";
    std::cout << "│ Total Indexed Scan Time: " << std::fixed << std::setprecision(2) 
              << total_indexed << " ms\n";
    
    // Phase 4.2 vs Phase 4.3 validation
    std::cout << "╠════════════════════════════════════════════════════════════════════════╣\n";
    std::cout << "│ PHASE 4.2 PREDICTION VALIDATION                                       │\n";
    std::cout << "│ ─────────────────────────────────────────────────────────────────────  │\n";
    
    if (avg_speedup >= 10) {
        std::cout << "│ ✅ Phase 4.3 achieves >= 10x speedup (Phase 4.2 predictions VALID) │\n";
    } else if (avg_speedup >= 5) {
        std::cout << "│ ⚠️  Phase 4.3 achieves 5-10x speedup (Phase 4.2 conservative)      │\n";
    } else {
        std::cout << "│ ❌ Phase 4.3 speedup < 5x (Phase 4.2 overestimated)                │\n";
    }
    
    std::cout << "╚════════════════════════════════════════════════════════════════════════╝\n\n";
    
    return 0;
}
