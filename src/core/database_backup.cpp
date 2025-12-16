#include "lyradb/database.h"
#include "lyradb/table.h"
#include "lyradb/query_execution_engine.h"
#include <stdexcept>
#include <memory>

namespace lyradb {

Database::Database(const std::string& path) : path_(path) {
    is_open_ = true;
    engine_ = std::make_unique<QueryExecutionEngine>(this);
}

Database::~Database() {
    if (is_open_) {
        close();
    }
}

void Database::create_table(const std::string& name, const Schema& schema) {
    if (tables_.find(name) != tables_.end()) {
        throw std::runtime_error("Table already exists: " + name);
    }
    
    tables_[name] = std::make_shared<Table>(name, schema);
}

std::shared_ptr<Table> Database::get_table(const std::string& name) {
    auto it = tables_.find(name);
    if (it == tables_.end()) {
        throw std::runtime_error("Table not found: " + name);
    }
    return it->second;
}

std::unique_ptr<QueryResult> Database::query(const std::string& sql) {
    if (engine_) {
        auto result = engine_->execute(sql);
        // TODO: Wrap in proper concrete QueryResult class
        return nullptr;
    }
    throw std::runtime_error("Query execution engine not initialized");
}

std::vector<std::string> Database::list_tables() const {
    std::vector<std::string> names;
    for (const auto& [name, _] : tables_) {
        names.push_back(name);
    }
    return names;
}

Schema Database::get_schema(const std::string& table_name) const {
    auto it = tables_.find(table_name);
    if (it == tables_.end()) {
        throw std::runtime_error("Table not found: " + table_name);
    }
    return it->second->get_schema();
}

void Database::close() {
    is_open_ = false;
}

}  // namespace lyradb
