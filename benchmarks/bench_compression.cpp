#include <benchmark/benchmark.h>
#include "lyradb/rle_compressor.h"
#include "lyradb/dict_compressor.h"
#include "lyradb/bitpacking_compressor.h"
#include "lyradb/delta_compressor.h"
#include <random>
#include <algorithm>

namespace lyradb {
namespace compression {
namespace benchmark_impl {

// RLE benchmarks
static void BenchRLECompress_HighRepetition(benchmark::State& state) {
    // Highly repetitive data
    std::vector<uint8_t> data(4096);
    for (size_t i = 0; i < data.size(); i += sizeof(int32_t)) {
        int32_t val = 42;
        std::memcpy(&data[i], &val, sizeof(int32_t));
    }
    
    for (auto _ : state) {
        auto compressed = RLECompressor::compress(
            data.data(), data.size(), sizeof(int32_t));
        benchmark::DoNotOptimize(compressed);
    }
}
BENCHMARK(BenchRLECompress_HighRepetition);

static void BenchRLECompress_RandomData(benchmark::State& state) {
    // Random data (worst case for RLE)
    std::vector<uint8_t> data(4096);
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<uint32_t> dist(0, UINT32_MAX);
    
    for (size_t i = 0; i < data.size(); i += sizeof(uint32_t)) {
        uint32_t val = dist(rng);
        std::memcpy(&data[i], &val, sizeof(uint32_t));
    }
    
    for (auto _ : state) {
        auto compressed = RLECompressor::compress(
            data.data(), data.size(), sizeof(uint32_t));
        benchmark::DoNotOptimize(compressed);
    }
}
BENCHMARK(BenchRLECompress_RandomData);

// Bitpacking benchmarks
static void BenchBitpackingCompress_SmallRange(benchmark::State& state) {
    // Values fitting in 4 bits
    std::vector<int64_t> values(1024);
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int64_t> dist(0, 15);
    std::generate(values.begin(), values.end(), [&]() { return dist(rng); });
    
    for (auto _ : state) {
        auto compressed = BitpackingCompressor::compress(
            values.data(), values.size());
        benchmark::DoNotOptimize(compressed);
    }
}
BENCHMARK(BenchBitpackingCompress_SmallRange);

static void BenchBitpackingCompress_LargeRange(benchmark::State& state) {
    // Full 64-bit range (worst case)
    std::vector<int64_t> values(1024);
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int64_t> dist(INT64_MIN, INT64_MAX);
    std::generate(values.begin(), values.end(), [&]() { return dist(rng); });
    
    for (auto _ : state) {
        auto compressed = BitpackingCompressor::compress(
            values.data(), values.size());
        benchmark::DoNotOptimize(compressed);
    }
}
BENCHMARK(BenchBitpackingCompress_LargeRange);

// Delta benchmarks
static void BenchDeltaCompress_SortedData(benchmark::State& state) {
    // Sorted data (ideal for delta)
    std::vector<int64_t> values(1024);
    for (size_t i = 0; i < values.size(); ++i) {
        values[i] = static_cast<int64_t>(i * 10);
    }
    
    for (auto _ : state) {
        auto compressed = DeltaCompressor::compress(
            values.data(), values.size());
        benchmark::DoNotOptimize(compressed);
    }
}
BENCHMARK(BenchDeltaCompress_SortedData);

static void BenchDeltaCompress_RandomData(benchmark::State& state) {
    // Random data
    std::vector<int64_t> values(1024);
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int64_t> dist(0, 10000);
    std::generate(values.begin(), values.end(), [&]() { return dist(rng); });
    
    for (auto _ : state) {
        auto compressed = DeltaCompressor::compress(
            values.data(), values.size());
        benchmark::DoNotOptimize(compressed);
    }
}
BENCHMARK(BenchDeltaCompress_RandomData);

// Dictionary benchmarks
static void BenchDictCompress_LowCardinality(benchmark::State& state) {
    // Low cardinality (ideal for dictionary)
    std::vector<std::string> values(1024);
    const std::string categories[] = {"apple", "banana", "cherry", "date"};
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 3);
    
    for (auto& val : values) {
        val = categories[dist(rng)];
    }
    
    for (auto _ : state) {
        auto compressed = DictionaryCompressor::compress(values);
        benchmark::DoNotOptimize(compressed);
    }
}
BENCHMARK(BenchDictCompress_LowCardinality);

static void BenchDictCompress_HighCardinality(benchmark::State& state) {
    // High cardinality (worst case for dictionary)
    std::vector<std::string> values(1024);
    std::mt19937 rng(std::random_device{}());
    
    for (size_t i = 0; i < values.size(); ++i) {
        values[i] = std::string("value_") + std::to_string(i);
    }
    
    for (auto _ : state) {
        auto compressed = DictionaryCompressor::compress(values);
        benchmark::DoNotOptimize(compressed);
    }
}
BENCHMARK(BenchDictCompress_HighCardinality);

// Decompression benchmarks
static void BenchRLEDecompress(benchmark::State& state) {
    std::vector<uint8_t> data(4096);
    for (size_t i = 0; i < data.size(); i += sizeof(int32_t)) {
        int32_t val = 42;
        std::memcpy(&data[i], &val, sizeof(int32_t));
    }
    auto compressed = RLECompressor::compress(
        data.data(), data.size(), sizeof(int32_t));
    
    for (auto _ : state) {
        auto decompressed = RLECompressor::decompress(
            compressed.data(), compressed.size(), sizeof(int32_t));
        benchmark::DoNotOptimize(decompressed);
    }
}
BENCHMARK(BenchRLEDecompress);

static void BenchBitpackingDecompress(benchmark::State& state) {
    std::vector<int64_t> values(1024);
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int64_t> dist(0, 15);
    std::generate(values.begin(), values.end(), [&]() { return dist(rng); });
    auto compressed = BitpackingCompressor::compress(
        values.data(), values.size());
    
    for (auto _ : state) {
        auto decompressed = BitpackingCompressor::decompress(
            compressed.data(), compressed.size());
        benchmark::DoNotOptimize(decompressed);
    }
}
BENCHMARK(BenchBitpackingDecompress);

} // namespace benchmark_impl
} // namespace compression
} // namespace lyradb

BENCHMARK_MAIN();
