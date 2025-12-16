/**
 * Standalone REST API Tests (No GTest required)
 * Tests the REST API server functionality
 */

#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <nlohmann/json.hpp>
#include "lyradb/rest_server.h"
#include "lyradb/database.h"

using json = nlohmann::json;
using namespace lyradb;
using namespace lyradb::server;

// Test counter
int passed = 0;
int failed = 0;

void assert_true(bool condition, const std::string& test_name) {
    if (condition) {
        std::cout << "✅ " << test_name << std::endl;
        passed++;
    } else {
        std::cout << "❌ " << test_name << std::endl;
        failed++;
    }
}

void assert_equal(const std::string& actual, const std::string& expected, const std::string& test_name) {
    if (actual == expected) {
        std::cout << "✅ " << test_name << std::endl;
        passed++;
    } else {
        std::cout << "❌ " << test_name << " (expected: " << expected << ", got: " << actual << ")" << std::endl;
        failed++;
    }
}

void assert_contains(const std::string& str, const std::string& substring, const std::string& test_name) {
    if (str.find(substring) != std::string::npos) {
        std::cout << "✅ " << test_name << std::endl;
        passed++;
    } else {
        std::cout << "❌ " << test_name << " (substring '" << substring << "' not found)" << std::endl;
        failed++;
    }
}

void assert_no_throw(const std::function<void()>& func, const std::string& test_name) {
    try {
        func();
        std::cout << "✅ " << test_name << std::endl;
        passed++;
    } catch (const std::exception& e) {
        std::cout << "❌ " << test_name << " (threw: " << e.what() << ")" << std::endl;
        failed++;
    }
}

// ============================================================================
// Section 1: Initialization Tests
// ============================================================================

void test_constructor_initializes_correctly() {
    RestServer srv("localhost", 9090);
    assert_true(!srv.is_running(), "Server is not running after construction");
}

void test_server_starts_with_database() {
    auto db = std::make_shared<Database>(":memory:");
    RestServer srv("127.0.0.1", 8080);
    srv.attach_database(db);
    
    assert_no_throw([&]() { srv.start(); }, "Server starts with attached database");
    assert_true(srv.is_running(), "Server is running after start");
}

void test_server_stops_gracefully() {
    auto db = std::make_shared<Database>(":memory:");
    RestServer srv("127.0.0.1", 8080);
    srv.attach_database(db);
    srv.start();
    
    srv.stop();
    assert_true(!srv.is_running(), "Server is not running after stop");
}

void test_database_attachment() {
    RestServer srv("127.0.0.1", 8080);
    auto db = std::make_shared<Database>(":memory:");
    
    assert_no_throw([&]() { srv.attach_database(db); }, "Database attachment succeeds");
}

// ============================================================================
// Section 2: JSON Response Formatting Tests
// ============================================================================

void test_error_response_format() {
    RestServer srv("127.0.0.1", 8080);
    std::string response = srv.json_error("Test error");
    
    auto json_response = json::parse(response);
    assert_true(!json_response["success"], "Error response has success=false");
    assert_equal(json_response["error"].get<std::string>(), "Test error", "Error response contains error message");
    assert_true(json_response.contains("timestamp"), "Error response has timestamp");
}

void test_success_response_format() {
    RestServer srv("127.0.0.1", 8080);
    std::string response = srv.json_success("Operation completed");
    
    auto json_response = json::parse(response);
    assert_true(json_response["success"], "Success response has success=true");
    assert_equal(json_response["message"].get<std::string>(), "Operation completed", "Success response contains message");
    assert_true(json_response.contains("timestamp"), "Success response has timestamp");
}

void test_result_to_json_format() {
    RestServer srv("127.0.0.1", 8080);
    
    std::vector<std::string> columns = {"id", "name", "age"};
    std::vector<std::vector<std::string>> rows = {
        {"1", "Alice", "30"},
        {"2", "Bob", "25"}
    };
    
    std::string response = srv.result_to_json(rows, columns);
    auto json_response = json::parse(response);
    
    assert_true(json_response["success"], "Result has success=true");
    assert_equal(std::to_string(json_response["row_count"].get<int>()), "2", "Result has correct row count");
    assert_equal(std::to_string(json_response["column_count"].get<int>()), "3", "Result has correct column count");
    assert_true(json_response.contains("data"), "Result has data array");
}

void test_result_to_json_empty() {
    RestServer srv("127.0.0.1", 8080);
    
    std::vector<std::string> columns = {"col1", "col2"};
    std::vector<std::vector<std::string>> rows;
    
    std::string response = srv.result_to_json(rows, columns);
    auto json_response = json::parse(response);
    
    assert_true(json_response["success"], "Empty result has success=true");
    assert_equal(std::to_string(json_response["row_count"].get<int>()), "0", "Empty result has 0 rows");
}

// ============================================================================
// Section 3: Query Handler Tests
// ============================================================================

void test_query_handler_empty_query() {
    auto db = std::make_shared<Database>(":memory:");
    RestServer srv("127.0.0.1", 8080);
    srv.attach_database(db);
    
    std::string response = srv.handle_query("");
    auto json_response = json::parse(response);
    
    assert_true(!json_response["success"], "Empty query returns error");
    assert_contains(json_response["error"].get<std::string>(), "empty", "Error mentions empty");
}

void test_query_handler_valid_sql() {
    auto db = std::make_shared<Database>(":memory:");
    RestServer srv("127.0.0.1", 8080);
    srv.attach_database(db);
    
    std::string response = srv.handle_query("SELECT * FROM test");
    auto json_response = json::parse(response);
    
    assert_true(json_response["success"], "Valid query succeeds");
    assert_true(json_response.contains("sql"), "Response contains SQL");
    assert_true(json_response.contains("rows_affected"), "Response contains rows_affected");
}

void test_query_handler_requires_database() {
    RestServer srv("127.0.0.1", 8080);
    
    std::string response = srv.handle_query("SELECT 1");
    auto json_response = json::parse(response);
    
    assert_true(!json_response["success"], "Query without database returns error");
}

// ============================================================================
// Section 4: Insert Handler Tests
// ============================================================================

void test_insert_handler_empty_table() {
    auto db = std::make_shared<Database>(":memory:");
    RestServer srv("127.0.0.1", 8080);
    srv.attach_database(db);
    
    json data = {{"name", "John"}};
    std::string response = srv.handle_insert("", data.dump());
    auto json_response = json::parse(response);
    
    assert_true(!json_response["success"], "Empty table name returns error");
}

void test_insert_handler_empty_data() {
    auto db = std::make_shared<Database>(":memory:");
    RestServer srv("127.0.0.1", 8080);
    srv.attach_database(db);
    
    std::string response = srv.handle_insert("users", "");
    auto json_response = json::parse(response);
    
    assert_true(!json_response["success"], "Empty data returns error");
}

void test_insert_handler_invalid_json() {
    auto db = std::make_shared<Database>(":memory:");
    RestServer srv("127.0.0.1", 8080);
    srv.attach_database(db);
    
    std::string response = srv.handle_insert("users", "{ invalid json }");
    auto json_response = json::parse(response);
    
    assert_true(!json_response["success"], "Invalid JSON returns error");
    assert_contains(json_response["error"].get<std::string>(), "JSON", "Error mentions JSON");
}

void test_insert_handler_single_row() {
    auto db = std::make_shared<Database>(":memory:");
    RestServer srv("127.0.0.1", 8080);
    srv.attach_database(db);
    
    json data = {{"id", 1}, {"name", "Alice"}};
    std::string response = srv.handle_insert("users", data.dump());
    auto json_response = json::parse(response);
    
    assert_true(json_response["success"], "Single row insert succeeds");
    assert_equal(std::to_string(json_response["rows_inserted"].get<int>()), "1", "Single row insert reports 1 row");
}

void test_insert_handler_multiple_rows() {
    auto db = std::make_shared<Database>(":memory:");
    RestServer srv("127.0.0.1", 8080);
    srv.attach_database(db);
    
    json data = json::array();
    data.push_back({{"id", 1}, {"name", "Alice"}});
    data.push_back({{"id", 2}, {"name", "Bob"}});
    data.push_back({{"id", 3}, {"name", "Charlie"}});
    
    std::string response = srv.handle_insert("users", data.dump());
    auto json_response = json::parse(response);
    
    assert_true(json_response["success"], "Multiple row insert succeeds");
    assert_equal(std::to_string(json_response["rows_inserted"].get<int>()), "3", "Multiple row insert reports correct count");
}

// ============================================================================
// Section 5: Table Listing Handler Tests
// ============================================================================

void test_list_tables_handler() {
    auto db = std::make_shared<Database>(":memory:");
    RestServer srv("127.0.0.1", 8080);
    srv.attach_database(db);
    
    std::string response = srv.handle_list_tables();
    auto json_response = json::parse(response);
    
    assert_true(json_response["success"], "List tables succeeds");
    assert_true(json_response.contains("tables"), "Response contains tables");
    assert_true(json_response["tables"].is_array(), "Tables is an array");
}

void test_list_tables_requires_database() {
    RestServer srv("127.0.0.1", 8080);
    
    std::string response = srv.handle_list_tables();
    auto json_response = json::parse(response);
    
    assert_true(!json_response["success"], "List tables without database returns error");
}

// ============================================================================
// Section 6: Schema Handler Tests
// ============================================================================

void test_get_table_schema_empty_table() {
    auto db = std::make_shared<Database>(":memory:");
    RestServer srv("127.0.0.1", 8080);
    srv.attach_database(db);
    
    std::string response = srv.handle_get_table_schema("");
    auto json_response = json::parse(response);
    
    assert_true(!json_response["success"], "Empty table name returns error");
}

void test_get_table_schema_valid_table() {
    auto db = std::make_shared<Database>(":memory:");
    RestServer srv("127.0.0.1", 8080);
    srv.attach_database(db);
    
    std::string response = srv.handle_get_table_schema("users");
    auto json_response = json::parse(response);
    
    assert_true(json_response["success"], "Get schema succeeds");
    assert_true(json_response.contains("columns"), "Response contains columns");
    assert_true(json_response.contains("row_count"), "Response contains row_count");
}

// ============================================================================
// Section 7: Status Handler Tests
// ============================================================================

void test_status_handler() {
    auto db = std::make_shared<Database>(":memory:");
    RestServer srv("127.0.0.1", 8080);
    srv.attach_database(db);
    
    std::string response = srv.handle_status();
    auto json_response = json::parse(response);
    
    assert_equal(json_response["server"].get<std::string>(), "LyraDB REST API", "Status contains server name");
    assert_equal(json_response["version"].get<std::string>(), "1.2.0", "Status contains correct version");
    assert_true(json_response["database_attached"], "Status shows database attached");
}

void test_status_handler_without_database() {
    RestServer srv("127.0.0.1", 8080);
    
    std::string response = srv.handle_status();
    auto json_response = json::parse(response);
    
    assert_true(!json_response["database_attached"], "Status shows database not attached");
}

void test_status_handler_running_state() {
    auto db = std::make_shared<Database>(":memory:");
    RestServer srv("127.0.0.1", 8080);
    srv.attach_database(db);
    
    srv.start();
    std::string response = srv.handle_status();
    auto json_response = json::parse(response);
    
    assert_equal(json_response["status"].get<std::string>(), "running", "Status shows running when started");
    
    srv.stop();
    response = srv.handle_status();
    json_response = json::parse(response);
    
    assert_equal(json_response["status"].get<std::string>(), "stopped", "Status shows stopped when stopped");
}

// ============================================================================
// Section 8: Integration Tests
// ============================================================================

void test_full_workflow() {
    auto db = std::make_shared<Database>(":memory:");
    RestServer srv("127.0.0.1", 8080);
    srv.attach_database(db);
    
    // Insert
    json insert_data = {{"id", 1}, {"name", "Test"}};
    std::string insert_response = srv.handle_insert("users", insert_data.dump());
    auto insert_json = json::parse(insert_response);
    assert_true(insert_json["success"], "Insert in workflow succeeds");
    
    // Get schema
    std::string schema_response = srv.handle_get_table_schema("users");
    auto schema_json = json::parse(schema_response);
    assert_true(schema_json["success"], "Get schema in workflow succeeds");
    
    // List tables
    std::string list_response = srv.handle_list_tables();
    auto list_json = json::parse(list_response);
    assert_true(list_json["success"], "List tables in workflow succeeds");
    
    // Query
    std::string query_response = srv.handle_query("SELECT * FROM users");
    auto query_json = json::parse(query_response);
    assert_true(query_json["success"], "Query in workflow succeeds");
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║      LyraDB REST API Standalone Test Suite                 ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
    
    // Section 1: Initialization
    std::cout << "📋 Section 1: Initialization & Lifecycle\n";
    test_constructor_initializes_correctly();
    test_server_starts_with_database();
    test_server_stops_gracefully();
    test_database_attachment();
    std::cout << "\n";
    
    // Section 2: JSON Formatting
    std::cout << "📋 Section 2: JSON Response Formatting\n";
    test_error_response_format();
    test_success_response_format();
    test_result_to_json_format();
    test_result_to_json_empty();
    std::cout << "\n";
    
    // Section 3: Query Handler
    std::cout << "📋 Section 3: Query Handler\n";
    test_query_handler_empty_query();
    test_query_handler_valid_sql();
    test_query_handler_requires_database();
    std::cout << "\n";
    
    // Section 4: Insert Handler
    std::cout << "📋 Section 4: Insert Handler\n";
    test_insert_handler_empty_table();
    test_insert_handler_empty_data();
    test_insert_handler_invalid_json();
    test_insert_handler_single_row();
    test_insert_handler_multiple_rows();
    std::cout << "\n";
    
    // Section 5: List Tables Handler
    std::cout << "📋 Section 5: Table Listing Handler\n";
    test_list_tables_handler();
    test_list_tables_requires_database();
    std::cout << "\n";
    
    // Section 6: Schema Handler
    std::cout << "📋 Section 6: Schema Handler\n";
    test_get_table_schema_empty_table();
    test_get_table_schema_valid_table();
    std::cout << "\n";
    
    // Section 7: Status Handler
    std::cout << "📋 Section 7: Status Handler\n";
    test_status_handler();
    test_status_handler_without_database();
    test_status_handler_running_state();
    std::cout << "\n";
    
    // Section 8: Integration
    std::cout << "📋 Section 8: Integration Tests\n";
    test_full_workflow();
    std::cout << "\n";
    
    // Summary
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                        Test Summary                         ║\n";
    std::cout << "╠════════════════════════════════════════════════════════════╣\n";
    std::cout << "║ ✅ Passed: " << passed << std::string(47 - std::to_string(passed).length(), ' ') << "║\n";
    std::cout << "║ ❌ Failed: " << failed << std::string(47 - std::to_string(failed).length(), ' ') << "║\n";
    std::cout << "║ Total:  " << (passed + failed) << std::string(48 - std::to_string(passed + failed).length(), ' ') << "║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
    
    if (failed == 0) {
        std::cout << "🎉 All tests passed!\n\n";
        return 0;
    } else {
        std::cout << "⚠️  " << failed << " test(s) failed\n\n";
        return 1;
    }
}
