#pragma once

#include "database.h"
#include "table.h"
#include "schema.h"
#include <string>
#include <memory>
#include <map>
#include <filesystem>

namespace lyradb {

/**
 * @brief DatabaseFile - File-based database persistence
 * 
 * This class provides file-based persistence for LyraDB databases.
 * Databases are saved as .db files that can be loaded later.
 * 
 * Usage:
 *   DatabaseFile db("mydata.db");
 *   db.execute("CREATE TABLE test (id INT32)");
 *   db.save();  // Saves to mydata.db
 *   
 *   DatabaseFile db2 = DatabaseFile::open("mydata.db");  // Load from file
 */
class DatabaseFile {
public:
    /**
     * @brief Create a new in-memory database with file persistence
     * @param filepath Path to .db file (will be created on save())
     */
    explicit DatabaseFile(const std::string& filepath);

    /**
     * @brief Open an existing .db file
     * @param filepath Path to existing .db file
     * @return DatabaseFile instance with loaded data
     * @throws std::runtime_error if file cannot be opened
     */
    static DatabaseFile open(const std::string& filepath);

    /**
     * @brief Execute SQL statement
     * @param sql SQL statement to execute
     * @return Query result
     */
    std::shared_ptr<QueryResult> execute(const std::string& sql);

    /**
     * @brief Save database to .db file
     * @throws std::runtime_error if save fails
     */
    void save();

    /**
     * @brief Save database to a specific location
     * @param filepath New path for .db file
     */
    void save_as(const std::string& filepath);

    /**
     * @brief Close and free resources
     */
    void close();

    /**
     * @brief Check if database is open
     */
    bool is_open() const;

    /**
     * @brief Get the current file path
     */
    const std::string& get_filepath() const;

    /**
     * @brief Get underlying database instance
     */
    Database& get_database();
    const Database& get_database() const;

    /**
     * @brief Get file size in bytes
     */
    size_t get_file_size() const;

    /**
     * @brief Get number of tables
     */
    size_t get_table_count() const;

    /**
     * @brief Get total number of rows across all tables
     */
    size_t get_total_rows() const;

    /**
     * @brief Compact the database file (remove deleted space)
     */
    void compact();

    /**
     * @brief Create backup of current .db file
     * @param backup_path Path for backup file
     */
    void backup(const std::string& backup_path);

    ~DatabaseFile();

private:
    std::string filepath_;
    std::unique_ptr<Database> db_;
    bool is_open_;
    bool modified_;

    /**
     * @brief Serialize database to binary format and write to file
     */
    void write_to_file();

    /**
     * @brief Read database from binary file and deserialize
     */
    void read_from_file();

    /**
     * @brief Get .db file header magic number
     */
    static constexpr uint32_t DB_MAGIC = 0x4C594244;  // "LYDB" in hex
    static constexpr uint32_t DB_VERSION = 1;
};

} // namespace lyradb
