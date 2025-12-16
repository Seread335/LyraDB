#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include "lyradb/rest_server.h"
#include "lyradb/database.h"
#include <memory>

using json = nlohmann::json;
using namespace lyradb;
using namespace lyradb::server;

class RestServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create in-memory database for testing
        db_ = std::make_shared<Database>(":memory:");
        server_ = std::make_unique<RestServer>("127.0.0.1", 8080);
        server_->attach_database(db_);
    }

    void TearDown() override {
        if (server_ && server_->is_running()) {
            server_->stop();
        }
    }

    std::shared_ptr<Database> db_;
    std::unique_ptr<RestServer> server_;
};

// ============================================================================
// Section 1: Initialization & Lifecycle Tests
// ============================================================================

TEST_F(RestServerTest, ConstructorInitializesCorrectly) {
    RestServer srv("localhost", 9090);
    ASSERT_FALSE(srv.is_running());
}

TEST_F(RestServerTest, ServerStartsWithAttachedDatabase) {
    ASSERT_NO_THROW(server_->start());
    ASSERT_TRUE(server_->is_running());
}

TEST_F(RestServerTest, ServerThrowsOnStartWithoutDatabase) {
    RestServer srv("localhost", 9090);
    ASSERT_THROW(srv.start(), std::runtime_error);
}

TEST_F(RestServerTest, ServerStopsGracefully) {
    server_->start();
    ASSERT_TRUE(server_->is_running());
    
    server_->stop();
    ASSERT_FALSE(server_->is_running());
}

TEST_F(RestServerTest, DatabaseAttachmentWorks) {
    auto db2 = std::make_shared<Database>(":memory:");
    ASSERT_NO_THROW(server_->attach_database(db2));
}

// ============================================================================
// Section 2: JSON Response Formatting Tests
// ============================================================================

TEST_F(RestServerTest, ErrorResponseFormatCorrect) {
    std::string response = server_->json_error("Test error message");
    
    auto json_response = json::parse(response);
    ASSERT_FALSE(json_response["success"]);
    ASSERT_EQ(json_response["error"].get<std::string>(), "Test error message");
    ASSERT_TRUE(json_response.contains("timestamp"));
}

TEST_F(RestServerTest, SuccessResponseFormatCorrect) {
    std::string response = server_->json_success("Operation completed");
    
    auto json_response = json::parse(response);
    ASSERT_TRUE(json_response["success"]);
    ASSERT_EQ(json_response["message"].get<std::string>(), "Operation completed");
    ASSERT_TRUE(json_response.contains("timestamp"));
}

TEST_F(RestServerTest, ResultToJsonFormatsCorrectly) {
    std::vector<std::string> columns = {"id", "name", "age"};
    std::vector<std::vector<std::string>> rows = {
        {"1", "Alice", "30"},
        {"2", "Bob", "25"},
        {"3", "Charlie", "35"}
    };
    
    std::string response = server_->result_to_json(rows, columns);
    auto json_response = json::parse(response);
    
    ASSERT_TRUE(json_response["success"]);
    ASSERT_EQ(json_response["row_count"], 3);
    ASSERT_EQ(json_response["column_count"], 3);
    ASSERT_EQ(json_response["columns"].size(), 3);
    ASSERT_EQ(json_response["data"].size(), 3);
    
    // Verify first row structure
    auto first_row = json_response["data"][0];
    ASSERT_EQ(first_row["id"].get<std::string>(), "1");
    ASSERT_EQ(first_row["name"].get<std::string>(), "Alice");
    ASSERT_EQ(first_row["age"].get<std::string>(), "30");
}

TEST_F(RestServerTest, ResultToJsonHandlesEmptyResults) {
    std::vector<std::string> columns = {"col1", "col2"};
    std::vector<std::vector<std::string>> rows;
    
    std::string response = server_->result_to_json(rows, columns);
    auto json_response = json::parse(response);
    
    ASSERT_TRUE(json_response["success"]);
    ASSERT_EQ(json_response["row_count"], 0);
    ASSERT_EQ(json_response["data"].size(), 0);
}

TEST_F(RestServerTest, ResultToJsonHandlesSpecialCharacters) {
    std::vector<std::string> columns = {"description", "notes"};
    std::vector<std::vector<std::string>> rows = {
        {"Special chars: \"quotes\" 'apostrophes'", "Newlines:\ntest"}
    };
    
    std::string response = server_->result_to_json(rows, columns);
    ASSERT_NO_THROW(json::parse(response));  // Should be valid JSON
}

// ============================================================================
// Section 3: Query Handler Tests
// ============================================================================

TEST_F(RestServerTest, QueryHandlerRejectsEmptyQuery) {
    std::string response = server_->handle_query("");
    auto json_response = json::parse(response);
    
    ASSERT_FALSE(json_response["success"]);
    ASSERT_TRUE(json_response["error"].get<std::string>().find("empty") != std::string::npos);
}

TEST_F(RestServerTest, QueryHandlerRequiresDatabase) {
    RestServer srv("localhost", 8080);
    std::string response = srv.handle_query("SELECT * FROM test");
    auto json_response = json::parse(response);
    
    ASSERT_FALSE(json_response["success"]);
}

TEST_F(RestServerTest, QueryHandlerProcessesValidSQL) {
    std::string query = "SELECT * FROM some_table WHERE id > 5";
    std::string response = server_->handle_query(query);
    
    auto json_response = json::parse(response);
    ASSERT_TRUE(json_response["success"]);
    ASSERT_EQ(json_response["sql"].get<std::string>(), query);
    ASSERT_TRUE(json_response.contains("timestamp"));
}

TEST_F(RestServerTest, QueryHandlerIncludesRowsAffected) {
    std::string response = server_->handle_query("DELETE FROM users WHERE age < 18");
    auto json_response = json::parse(response);
    
    ASSERT_TRUE(json_response.contains("rows_affected"));
}

// ============================================================================
// Section 4: Insert Handler Tests
// ============================================================================

TEST_F(RestServerTest, InsertHandlerRejectsEmptyTableName) {
    json data = {{"name", "John"}, {"age", 30}};
    std::string response = server_->handle_insert("", data.dump());
    auto json_response = json::parse(response);
    
    ASSERT_FALSE(json_response["success"]);
    ASSERT_TRUE(json_response["error"].get<std::string>().find("Table name") != std::string::npos);
}

TEST_F(RestServerTest, InsertHandlerRejectsEmptyData) {
    std::string response = server_->handle_insert("users", "");
    auto json_response = json::parse(response);
    
    ASSERT_FALSE(json_response["success"]);
    ASSERT_TRUE(json_response["error"].get<std::string>().find("empty") != std::string::npos);
}

TEST_F(RestServerTest, InsertHandlerRejectsInvalidJSON) {
    std::string response = server_->handle_insert("users", "{ invalid json }");
    auto json_response = json::parse(response);
    
    ASSERT_FALSE(json_response["success"]);
    ASSERT_TRUE(json_response["error"].get<std::string>().find("JSON") != std::string::npos);
}

TEST_F(RestServerTest, InsertHandlerSingleRowObject) {
    json data = {{"id", 1}, {"name", "Alice"}, {"age", 30}};
    std::string response = server_->handle_insert("users", data.dump());
    auto json_response = json::parse(response);
    
    ASSERT_TRUE(json_response["success"]);
    ASSERT_EQ(json_response["table"].get<std::string>(), "users");
    ASSERT_EQ(json_response["rows_inserted"].get<int>(), 1);
}

TEST_F(RestServerTest, InsertHandlerMultipleRows) {
    json data = json::array();
    data.push_back({{"id", 1}, {"name", "Alice"}});
    data.push_back({{"id", 2}, {"name", "Bob"}});
    data.push_back({{"id", 3}, {"name", "Charlie"}});
    
    std::string response = server_->handle_insert("users", data.dump());
    auto json_response = json::parse(response);
    
    ASSERT_TRUE(json_response["success"]);
    ASSERT_EQ(json_response["rows_inserted"].get<int>(), 3);
}

TEST_F(RestServerTest, InsertHandlerRejectsNonObjectArray) {
    json data = json::array({1, 2, 3});  // Array of scalars, not objects
    std::string response = server_->handle_insert("users", data.dump());
    auto json_response = json::parse(response);
    
    ASSERT_FALSE(json_response["success"]);
}

TEST_F(RestServerTest, InsertHandlerRequiresDatabase) {
    RestServer srv("localhost", 8080);
    json data = {{"id", 1}};
    std::string response = srv.handle_insert("users", data.dump());
    auto json_response = json::parse(response);
    
    ASSERT_FALSE(json_response["success"]);
}

// ============================================================================
// Section 5: Table Listing Handler Tests
// ============================================================================

TEST_F(RestServerTest, ListTablesHandlerSucceeds) {
    std::string response = server_->handle_list_tables();
    auto json_response = json::parse(response);
    
    ASSERT_TRUE(json_response["success"]);
    ASSERT_TRUE(json_response.contains("tables"));
    ASSERT_TRUE(json_response["tables"].is_array());
    ASSERT_TRUE(json_response.contains("table_count"));
}

TEST_F(RestServerTest, ListTablesHandlerRequiresDatabase) {
    RestServer srv("localhost", 8080);
    std::string response = srv.handle_list_tables();
    auto json_response = json::parse(response);
    
    ASSERT_FALSE(json_response["success"]);
}

// ============================================================================
// Section 6: Schema Handler Tests
// ============================================================================

TEST_F(RestServerTest, GetTableSchemaHandlerRejectsEmptyTableName) {
    std::string response = server_->handle_get_table_schema("");
    auto json_response = json::parse(response);
    
    ASSERT_FALSE(json_response["success"]);
    ASSERT_TRUE(json_response["error"].get<std::string>().find("Table name") != std::string::npos);
}

TEST_F(RestServerTest, GetTableSchemaHandlerRequiresDatabase) {
    RestServer srv("localhost", 8080);
    std::string response = srv.handle_get_table_schema("users");
    auto json_response = json::parse(response);
    
    ASSERT_FALSE(json_response["success"]);
}

TEST_F(RestServerTest, GetTableSchemaHandlerReturnsCorrectFormat) {
    std::string response = server_->handle_get_table_schema("users");
    auto json_response = json::parse(response);
    
    ASSERT_TRUE(json_response["success"]);
    ASSERT_EQ(json_response["table"].get<std::string>(), "users");
    ASSERT_TRUE(json_response.contains("columns"));
    ASSERT_TRUE(json_response.contains("row_count"));
}

// ============================================================================
// Section 7: Status Handler Tests
// ============================================================================

TEST_F(RestServerTest, StatusHandlerReturnsCorrectInfo) {
    server_->start();
    std::string response = server_->handle_status();
    auto json_response = json::parse(response);
    
    ASSERT_EQ(json_response["server"].get<std::string>(), "LyraDB REST API");
    ASSERT_EQ(json_response["version"].get<std::string>(), "1.2.0");
    ASSERT_EQ(json_response["status"].get<std::string>(), "running");
    ASSERT_TRUE(json_response["database_attached"]);
}

TEST_F(RestServerTest, StatusHandlerWithoutDatabase) {
    RestServer srv("localhost", 8080);
    std::string response = srv.handle_status();
    auto json_response = json::parse(response);
    
    ASSERT_FALSE(json_response["database_attached"]);
}

TEST_F(RestServerTest, StatusHandlerIncludesTimestamp) {
    std::string response = server_->handle_status();
    auto json_response = json::parse(response);
    
    ASSERT_TRUE(json_response.contains("timestamp"));
    ASSERT_TRUE(json_response["timestamp"].is_number());
}

// ============================================================================
// Section 8: Edge Cases & Stress Tests
// ============================================================================

TEST_F(RestServerTest, HandlesLargeDatasets) {
    std::vector<std::string> columns = {"id", "value"};
    std::vector<std::vector<std::string>> rows;
    
    // Generate 10,000 rows
    for (int i = 0; i < 10000; ++i) {
        rows.push_back({std::to_string(i), "value_" + std::to_string(i)});
    }
    
    std::string response = server_->result_to_json(rows, columns);
    auto json_response = json::parse(response);
    
    ASSERT_TRUE(json_response["success"]);
    ASSERT_EQ(json_response["row_count"], 10000);
}

TEST_F(RestServerTest, HandlesManyColumns) {
    std::vector<std::string> columns;
    for (int i = 0; i < 100; ++i) {
        columns.push_back("col_" + std::to_string(i));
    }
    
    std::vector<std::string> row(100, "value");
    std::vector<std::vector<std::string>> rows = {row};
    
    std::string response = server_->result_to_json(rows, columns);
    auto json_response = json::parse(response);
    
    ASSERT_TRUE(json_response["success"]);
    ASSERT_EQ(json_response["column_count"], 100);
}

TEST_F(RestServerTest, HandlesUnicodeInData) {
    json data = json::object();
    data["name"] = "François";
    data["city"] = "São Paulo";
    data["country"] = "日本";
    
    std::string response = server_->handle_insert("users", data.dump());
    ASSERT_NO_THROW(json::parse(response));
}

TEST_F(RestServerTest, JSONResponsesAlwaysValid) {
    // Ensure all handlers return valid JSON
    ASSERT_NO_THROW(json::parse(server_->json_error("test")));
    ASSERT_NO_THROW(json::parse(server_->json_success("test")));
    ASSERT_NO_THROW(json::parse(server_->handle_query("SELECT 1")));
    ASSERT_NO_THROW(json::parse(server_->handle_insert("t", "{}")));
    ASSERT_NO_THROW(json::parse(server_->handle_list_tables()));
    ASSERT_NO_THROW(json::parse(server_->handle_get_table_schema("t")));
    ASSERT_NO_THROW(json::parse(server_->handle_status()));
}

TEST_F(RestServerTest, ConcurrentInserts) {
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&, i]() {
            json data = {{"id", i}, {"value", "thread_" + std::to_string(i)}};
            std::string response = server_->handle_insert("concurrent_test", data.dump());
            ASSERT_NO_THROW(json::parse(response));
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
}

// ============================================================================
// Section 9: Error Recovery Tests
// ============================================================================

TEST_F(RestServerTest, RecoveryFromInvalidJSON) {
    // First call with invalid JSON
    std::string bad_response = server_->handle_insert("users", "{ bad json");
    auto bad_json = json::parse(bad_response);
    ASSERT_FALSE(bad_json["success"]);
    
    // Second call with valid JSON should work
    json good_data = {{"id", 1}};
    std::string good_response = server_->handle_insert("users", good_data.dump());
    auto good_json = json::parse(good_response);
    ASSERT_TRUE(good_json["success"]);
}

TEST_F(RestServerTest, MultipleStartStopCycles) {
    for (int i = 0; i < 3; ++i) {
        ASSERT_NO_THROW(server_->start());
        ASSERT_TRUE(server_->is_running());
        
        ASSERT_NO_THROW(server_->stop());
        ASSERT_FALSE(server_->is_running());
    }
}

// ============================================================================
// Section 10: Integration Tests
// ============================================================================

TEST_F(RestServerTest, FullWorkflowInsertAndQuery) {
    // Insert data
    json insert_data = {{"id", 1}, {"name", "Test User"}};
    std::string insert_response = server_->handle_insert("users", insert_data.dump());
    auto insert_json = json::parse(insert_response);
    ASSERT_TRUE(insert_json["success"]);
    
    // Get table info
    std::string schema_response = server_->handle_get_table_schema("users");
    auto schema_json = json::parse(schema_response);
    ASSERT_TRUE(schema_json["success"]);
    
    // List tables
    std::string list_response = server_->handle_list_tables();
    auto list_json = json::parse(list_response);
    ASSERT_TRUE(list_json["success"]);
    
    // Query table
    std::string query_response = server_->handle_query("SELECT * FROM users");
    auto query_json = json::parse(query_response);
    ASSERT_TRUE(query_json["success"]);
}

TEST_F(RestServerTest, ServerStatusThroughoutLifecycle) {
    // Initial status
    std::string status1 = server_->handle_status();
    auto json1 = json::parse(status1);
    ASSERT_EQ(json1["status"].get<std::string>(), "stopped");
    
    // After start
    server_->start();
    std::string status2 = server_->handle_status();
    auto json2 = json::parse(status2);
    ASSERT_EQ(json2["status"].get<std::string>(), "running");
    
    // After stop
    server_->stop();
    std::string status3 = server_->handle_status();
    auto json3 = json::parse(status3);
    ASSERT_EQ(json3["status"].get<std::string>(), "stopped");
}

