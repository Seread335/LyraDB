#include "gtest/gtest.h"
#include "lyradb/aggregator.h"
#include <vector>
#include <string>

using namespace lyradb;

class AggregatorTestFixture : public ::testing::Test {
protected:
    std::vector<std::string> empty_values;
    std::vector<std::string> single_value;
    std::vector<std::string> numeric_values;
    std::vector<std::string> mixed_values;
    std::vector<std::string> null_values;
    
    void SetUp() override {
        empty_values = {};
        single_value = {"42"};
        numeric_values = {"10", "20", "30", "40", "50"};
        mixed_values = {"10", "abc", "20", "", "30", "NULL", "40"};
        null_values = {"NULL", "NULL", "", "NULL"};
    }
};

// ============================================================================
// COUNT TESTS
// ============================================================================

TEST_F(AggregatorTestFixture, COUNT_EmptyList) {
    EXPECT_EQ(Aggregator::count(empty_values), 0);
}

TEST_F(AggregatorTestFixture, COUNT_SingleValue) {
    EXPECT_EQ(Aggregator::count(single_value), 1);
}

TEST_F(AggregatorTestFixture, COUNT_MultipleValues) {
    EXPECT_EQ(Aggregator::count(numeric_values), 5);
}

TEST_F(AggregatorTestFixture, COUNT_WithNullAndEmpty) {
    EXPECT_EQ(Aggregator::count(mixed_values), 4);  // Only "10", "20", "30", "40"
}

TEST_F(AggregatorTestFixture, COUNT_AllNull) {
    EXPECT_EQ(Aggregator::count(null_values), 0);
}

// ============================================================================
// SUM TESTS
// ============================================================================

TEST_F(AggregatorTestFixture, SUM_EmptyList) {
    EXPECT_EQ(Aggregator::sum(empty_values), 0.0);
}

TEST_F(AggregatorTestFixture, SUM_SingleValue) {
    EXPECT_EQ(Aggregator::sum(single_value), 42.0);
}

TEST_F(AggregatorTestFixture, SUM_MultipleValues) {
    EXPECT_EQ(Aggregator::sum(numeric_values), 150.0);  // 10+20+30+40+50
}

TEST_F(AggregatorTestFixture, SUM_WithNullAndEmpty) {
    EXPECT_EQ(Aggregator::sum(mixed_values), 100.0);  // 10+20+30+40
}

TEST_F(AggregatorTestFixture, SUM_WithNegatives) {
    std::vector<std::string> negative_values = {"-10", "20", "-5", "15"};
    EXPECT_EQ(Aggregator::sum(negative_values), 20.0);  // -10+20-5+15
}

TEST_F(AggregatorTestFixture, SUM_AllNull) {
    EXPECT_EQ(Aggregator::sum(null_values), 0.0);
}

// ============================================================================
// AVG TESTS
// ============================================================================

TEST_F(AggregatorTestFixture, AVG_EmptyList) {
    EXPECT_EQ(Aggregator::avg(empty_values), 0.0);
}

TEST_F(AggregatorTestFixture, AVG_SingleValue) {
    EXPECT_EQ(Aggregator::avg(single_value), 42.0);
}

TEST_F(AggregatorTestFixture, AVG_MultipleValues) {
    EXPECT_EQ(Aggregator::avg(numeric_values), 30.0);  // (10+20+30+40+50)/5
}

TEST_F(AggregatorTestFixture, AVG_WithNullAndEmpty) {
    EXPECT_EQ(Aggregator::avg(mixed_values), 25.0);  // (10+20+30+40)/4
}

TEST_F(AggregatorTestFixture, AVG_WithDecimals) {
    std::vector<std::string> decimal_values = {"10.5", "20.5", "30.0"};
    EXPECT_NEAR(Aggregator::avg(decimal_values), 20.333333, 0.0001);
}

TEST_F(AggregatorTestFixture, AVG_AllNull) {
    EXPECT_EQ(Aggregator::avg(null_values), 0.0);
}

// ============================================================================
// MIN TESTS
// ============================================================================

TEST_F(AggregatorTestFixture, MIN_EmptyList) {
    EXPECT_EQ(Aggregator::min_value(empty_values), 0.0);
}

TEST_F(AggregatorTestFixture, MIN_SingleValue) {
    EXPECT_EQ(Aggregator::min_value(single_value), 42.0);
}

TEST_F(AggregatorTestFixture, MIN_MultipleValues) {
    EXPECT_EQ(Aggregator::min_value(numeric_values), 10.0);
}

TEST_F(AggregatorTestFixture, MIN_WithNullAndEmpty) {
    EXPECT_EQ(Aggregator::min_value(mixed_values), 10.0);
}

TEST_F(AggregatorTestFixture, MIN_WithNegatives) {
    std::vector<std::string> negative_values = {"10", "-50", "20", "-5"};
    EXPECT_EQ(Aggregator::min_value(negative_values), -50.0);
}

TEST_F(AggregatorTestFixture, MIN_AllNull) {
    EXPECT_EQ(Aggregator::min_value(null_values), 0.0);
}

// ============================================================================
// MAX TESTS
// ============================================================================

TEST_F(AggregatorTestFixture, MAX_EmptyList) {
    EXPECT_EQ(Aggregator::max_value(empty_values), 0.0);
}

TEST_F(AggregatorTestFixture, MAX_SingleValue) {
    EXPECT_EQ(Aggregator::max_value(single_value), 42.0);
}

TEST_F(AggregatorTestFixture, MAX_MultipleValues) {
    EXPECT_EQ(Aggregator::max_value(numeric_values), 50.0);
}

TEST_F(AggregatorTestFixture, MAX_WithNullAndEmpty) {
    EXPECT_EQ(Aggregator::max_value(mixed_values), 40.0);
}

TEST_F(AggregatorTestFixture, MAX_WithNegatives) {
    std::vector<std::string> negative_values = {"-10", "-50", "-5", "-30"};
    EXPECT_EQ(Aggregator::max_value(negative_values), -5.0);
}

TEST_F(AggregatorTestFixture, MAX_AllNull) {
    EXPECT_EQ(Aggregator::max_value(null_values), 0.0);
}

// ============================================================================
// COMBINATION TESTS (Testing multiple aggregates on same data)
// ============================================================================

TEST_F(AggregatorTestFixture, AllAggregates_OnSameData) {
    auto count = Aggregator::count(numeric_values);
    auto sum = Aggregator::sum(numeric_values);
    auto avg = Aggregator::avg(numeric_values);
    auto min = Aggregator::min_value(numeric_values);
    auto max = Aggregator::max_value(numeric_values);
    
    EXPECT_EQ(count, 5);
    EXPECT_EQ(sum, 150.0);
    EXPECT_EQ(avg, 30.0);
    EXPECT_EQ(min, 10.0);
    EXPECT_EQ(max, 50.0);
}

TEST_F(AggregatorTestFixture, AllAggregates_WithMixedValues) {
    auto count = Aggregator::count(mixed_values);
    auto sum = Aggregator::sum(mixed_values);
    auto avg = Aggregator::avg(mixed_values);
    auto min = Aggregator::min_value(mixed_values);
    auto max = Aggregator::max_value(mixed_values);
    
    EXPECT_EQ(count, 4);
    EXPECT_EQ(sum, 100.0);
    EXPECT_EQ(avg, 25.0);
    EXPECT_EQ(min, 10.0);
    EXPECT_EQ(max, 40.0);
}

// ============================================================================
// EDGE CASE TESTS
// ============================================================================

TEST_F(AggregatorTestFixture, LargeNumbers) {
    std::vector<std::string> large_values = {"1000000", "2000000", "3000000"};
    EXPECT_EQ(Aggregator::sum(large_values), 6000000.0);
    EXPECT_EQ(Aggregator::avg(large_values), 2000000.0);
}

TEST_F(AggregatorTestFixture, VerySmallNumbers) {
    std::vector<std::string> small_values = {"0.001", "0.002", "0.003"};
    EXPECT_NEAR(Aggregator::sum(small_values), 0.006, 0.0001);
    EXPECT_NEAR(Aggregator::avg(small_values), 0.002, 0.0001);
}

TEST_F(AggregatorTestFixture, MixedPositiveNegative) {
    std::vector<std::string> mixed = {"100", "-50", "25", "-75", "50"};
    EXPECT_EQ(Aggregator::sum(mixed), 50.0);
    EXPECT_EQ(Aggregator::count(mixed), 5);
    EXPECT_EQ(Aggregator::min_value(mixed), -75.0);
    EXPECT_EQ(Aggregator::max_value(mixed), 100.0);
}

TEST_F(AggregatorTestFixture, StringConversion) {
    std::string str_count = Aggregator::to_string(5.0);
    std::string str_decimal = Aggregator::to_string(5.5);
    
    EXPECT_EQ(str_count, "5");
    EXPECT_NE(str_decimal, "5");  // Should have decimal part
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
