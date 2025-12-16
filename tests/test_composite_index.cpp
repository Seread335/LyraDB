/**
 * @file test_composite_index.cpp
 * @brief Unit tests for composite (multi-column) hash index functionality (Phase 4.1.2)
 * 
 * Tests:
 * - Building composite indexes on 2+ columns
 * - Exact-match lookups on composite keys
 * - Correctness with various data types
 * - Composite key hashing
 */

#include "lyradb/database.h"
#include "lyradb/schema.h"
#include <gtest/gtest.h>
#include <iostream>

using namespace lyradb;
using namespace lyradb::index;

class CompositeIndexTest : public ::testing::Test {
protected:
    void SetUp() override {
        db = std::make_unique<Database>();
    }
    
    void TearDown() override {
        db.reset();
    }
    
    std::unique_ptr<Database> db;
};

/**
 * Test: Create composite index on 2 columns and lookup by composite key
 */
TEST_F(CompositeIndexTest, CreateAndLookupTwoColumn) {
    // Create schema
    Schema schema;
    schema.add_column("country", DataType::VARCHAR);
    schema.add_column("city", DataType::VARCHAR);
    schema.add_column("population", DataType::INT);
    
    // Create table
    db->create_table("cities", schema);
    
    // Insert sample data
    db->insert_row("cities", {"USA", "New York", "8000000"});
    db->insert_row("cities", {"USA", "Los Angeles", "4000000"});
    db->insert_row("cities", {"Canada", "Toronto", "2930000"});
    db->insert_row("cities", {"Canada", "Vancouver", "675000"});
    db->insert_row("cities", {"USA", "Chicago", "2700000"});
    
    // Create composite index on (country, city)
    db->execute_query("CREATE INDEX idx_country_city ON cities (country, city)");
    
    // Lookup rows where country='USA' AND city='Los Angeles'
    auto result = db->execute_query("SELECT * FROM cities WHERE country='USA' AND city='Los Angeles'");
    
    // Should find exactly one row
    EXPECT_EQ(result.row_count(), 1);
    EXPECT_EQ(result.get_row(0)[0], "USA");
    EXPECT_EQ(result.get_row(0)[1], "Los Angeles");
    EXPECT_EQ(result.get_row(0)[2], "4000000");
}

/**
 * Test: Create composite index on 3 columns
 */
TEST_F(CompositeIndexTest, CreateAndLookupThreeColumn) {
    // Create schema with 3 indexed columns
    Schema schema;
    schema.add_column("continent", DataType::VARCHAR);
    schema.add_column("country", DataType::VARCHAR);
    schema.add_column("city", DataType::VARCHAR);
    
    // Create table
    db->create_table("world_cities", schema);
    
    // Insert sample data
    db->insert_row("world_cities", {"North America", "USA", "New York"});
    db->insert_row("world_cities", {"North America", "USA", "Los Angeles"});
    db->insert_row("world_cities", {"North America", "Canada", "Toronto"});
    db->insert_row("world_cities", {"Europe", "France", "Paris"});
    db->insert_row("world_cities", {"Europe", "France", "Lyon"});
    
    // Create composite index on (continent, country, city)
    db->execute_query("CREATE INDEX idx_location ON world_cities (continent, country, city)");
    
    // Lookup specific location
    auto result = db->execute_query(
        "SELECT * FROM world_cities WHERE continent='Europe' AND country='France' AND city='Paris'"
    );
    
    // Should find exactly one row
    EXPECT_EQ(result.row_count(), 1);
    EXPECT_EQ(result.get_row(0)[0], "Europe");
    EXPECT_EQ(result.get_row(0)[1], "France");
    EXPECT_EQ(result.get_row(0)[2], "Paris");
}

/**
 * Test: Composite index lookup returns no results for non-existent key
 */
TEST_F(CompositeIndexTest, CompositeKeyNotFound) {
    // Create schema
    Schema schema;
    schema.add_column("country", DataType::VARCHAR);
    schema.add_column("city", DataType::VARCHAR);
    
    // Create table
    db->create_table("cities", schema);
    
    // Insert data
    db->insert_row("cities", {"USA", "New York"});
    db->insert_row("cities", {"USA", "Los Angeles"});
    
    // Create composite index
    db->execute_query("CREATE INDEX idx_cc ON cities (country, city)");
    
    // Lookup non-existent combination
    auto result = db->execute_query("SELECT * FROM cities WHERE country='Canada' AND city='Toronto'");
    
    // Should return empty result set
    EXPECT_EQ(result.row_count(), 0);
}

/**
 * Test: Composite index with NULL values
 */
TEST_F(CompositeIndexTest, CompositeIndexWithNull) {
    // Create schema
    Schema schema;
    schema.add_column("col1", DataType::VARCHAR);
    schema.add_column("col2", DataType::VARCHAR);
    
    // Create table
    db->create_table("test_null", schema);
    
    // Insert data with empty values
    db->insert_row("test_null", {"A", "B"});
    db->insert_row("test_null", {"A", ""});
    db->insert_row("test_null", {"", "B"});
    
    // Create composite index
    db->execute_query("CREATE INDEX idx_null ON test_null (col1, col2)");
    
    // Lookup with empty value
    auto result = db->execute_query("SELECT * FROM test_null WHERE col1='A' AND col2=''");
    
    // Should find the row
    EXPECT_EQ(result.row_count(), 1);
    EXPECT_EQ(result.get_row(0)[0], "A");
    EXPECT_EQ(result.get_row(0)[1], "");
}

/**
 * Test: Multiple composite indexes on same table
 */
TEST_F(CompositeIndexTest, MultipleCompositeIndexes) {
    // Create schema
    Schema schema;
    schema.add_column("country", DataType::VARCHAR);
    schema.add_column("city", DataType::VARCHAR);
    schema.add_column("year", DataType::INT);
    
    // Create table
    db->create_table("statistics", schema);
    
    // Insert data
    db->insert_row("statistics", {"USA", "New York", "2020"});
    db->insert_row("statistics", {"USA", "New York", "2021"});
    db->insert_row("statistics", {"USA", "Los Angeles", "2020"});
    db->insert_row("statistics", {"Canada", "Toronto", "2020"});
    
    // Create two different composite indexes
    db->execute_query("CREATE INDEX idx_cc ON statistics (country, city)");
    db->execute_query("CREATE INDEX idx_cy ON statistics (country, year)");
    
    // Query using first index
    auto result1 = db->execute_query("SELECT * FROM statistics WHERE country='USA' AND city='New York'");
    EXPECT_EQ(result1.row_count(), 2);
    
    // Query using second index
    auto result2 = db->execute_query("SELECT * FROM statistics WHERE country='USA' AND year='2020'");
    EXPECT_EQ(result2.row_count(), 1);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
