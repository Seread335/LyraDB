#pragma once

#include <string>
#include <memory>
#include <functional>
#include "lyradb/database.h"

namespace lyradb::server {

/**
 * REST API Server for LyraDB
 * 
 * Provides HTTP endpoints for database operations:
 * - POST   /api/v1/tables/:table_name/insert    - Insert data
 * - GET    /api/v1/tables/:table_name           - Query table
 * - POST   /api/v1/query                         - Execute SQL query
 * - GET    /api/v1/status                        - Server status
 * - POST   /api/v1/indexes                       - Create index
 * - DELETE /api/v1/indexes/:index_name           - Drop index
 */
class RestServer {
public:
    RestServer(const std::string& host, int port);
    ~RestServer() = default;

    /**
     * Start the HTTP server
     * Blocks until server is stopped
     */
    void start();

    /**
     * Stop the HTTP server
     */
    void stop();

    /**
     * Attach a database instance to the server
     * Server will manage queries against this database
     */
    void attach_database(std::shared_ptr<Database> db);

private:
    std::string host_;
    int port_;
    std::shared_ptr<Database> db_;

    // HTTP request handlers
    void handle_insert_data(const std::string& table_name, const std::string& body);
    void handle_query_table(const std::string& table_name);
    void handle_sql_query(const std::string& sql);
    void handle_create_index(const std::string& body);
    void handle_drop_index(const std::string& index_name);
    void handle_status();
};

} // namespace lyradb::server
