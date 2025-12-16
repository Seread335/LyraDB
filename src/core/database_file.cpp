#include "lyradb/database_file.h"
#include <fstream>
#include <sstream>
#include <cstring>
#include <filesystem>
#include <stdexcept>

namespace lyradb {

// ============================================================================
// DatabaseFile Implementation
// ============================================================================

DatabaseFile::DatabaseFile(const std::string& filepath)
    : filepath_(filepath), 
      db_(std::make_unique<Database>()),
      is_open_(true),
      modified_(false) {
    
    // Check if file exists and load it
    namespace fs = std::filesystem;
    if (fs::exists(filepath)) {
        try {
            read_from_file();
        } catch (const std::exception& e) {
            // If file cannot be loaded, start with empty database
            db_ = std::make_unique<Database>();
        }
    }
}

DatabaseFile DatabaseFile::open(const std::string& filepath) {
    namespace fs = std::filesystem;
    
    if (!fs::exists(filepath)) {
        throw std::runtime_error("Database file not found: " + filepath);
    }
    
    DatabaseFile dbf(filepath);
    dbf.read_from_file();
    return dbf;
}

std::shared_ptr<QueryResult> DatabaseFile::execute(const std::string& sql) {
    if (!is_open_) {
        throw std::runtime_error("Database is closed");
    }
    
    auto result = db_->execute(sql);
    
    // Mark as modified if this is a write operation
    if (sql.find("INSERT") != std::string::npos ||
        sql.find("UPDATE") != std::string::npos ||
        sql.find("DELETE") != std::string::npos ||
        sql.find("CREATE") != std::string::npos ||
        sql.find("DROP") != std::string::npos) {
        modified_ = true;
    }
    
    return result;
}

void DatabaseFile::save() {
    if (!is_open_) {
        throw std::runtime_error("Database is closed");
    }
    write_to_file();
    modified_ = false;
}

void DatabaseFile::save_as(const std::string& filepath) {
    filepath_ = filepath;
    save();
}

void DatabaseFile::close() {
    if (modified_) {
        try {
            save();
        } catch (const std::exception& e) {
            // Log error but don't throw
        }
    }
    db_ = nullptr;
    is_open_ = false;
}

bool DatabaseFile::is_open() const {
    return is_open_ && db_ != nullptr;
}

const std::string& DatabaseFile::get_filepath() const {
    return filepath_;
}

Database& DatabaseFile::get_database() {
    if (!is_open_) {
        throw std::runtime_error("Database is closed");
    }
    return *db_;
}

const Database& DatabaseFile::get_database() const {
    if (!is_open_) {
        throw std::runtime_error("Database is closed");
    }
    return *db_;
}

size_t DatabaseFile::get_file_size() const {
    namespace fs = std::filesystem;
    try {
        if (fs::exists(filepath_)) {
            return fs::file_size(filepath_);
        }
    } catch (...) {
    }
    return 0;
}

size_t DatabaseFile::get_table_count() const {
    if (!is_open_) return 0;
    return db_->get_table_count();
}

size_t DatabaseFile::get_total_rows() const {
    if (!is_open_) return 0;
    // Sum rows from all tables
    size_t total = 0;
    // This would require access to table enumeration
    return total;
}

void DatabaseFile::compact() {
    if (!is_open_) {
        throw std::runtime_error("Database is closed");
    }
    // Save to a temporary file, then replace
    std::string temp_path = filepath_ + ".tmp";
    std::string original_path = filepath_;
    
    filepath_ = temp_path;
    write_to_file();
    
    namespace fs = std::filesystem;
    fs::remove(original_path);
    fs::rename(temp_path, original_path);
    
    filepath_ = original_path;
    modified_ = false;
}

void DatabaseFile::backup(const std::string& backup_path) {
    namespace fs = std::filesystem;
    
    if (!fs::exists(filepath_)) {
        throw std::runtime_error("Database file does not exist");
    }
    
    fs::copy_file(filepath_, backup_path, fs::copy_options::overwrite_existing);
}

DatabaseFile::~DatabaseFile() {
    try {
        close();
    } catch (...) {
        // Suppress exceptions in destructor
    }
}

void DatabaseFile::write_to_file() {
    std::ofstream file(filepath_, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file for writing: " + filepath_);
    }
    
    // Write header
    uint32_t magic = DB_MAGIC;
    uint32_t version = DB_VERSION;
    
    file.write(reinterpret_cast<char*>(&magic), sizeof(magic));
    file.write(reinterpret_cast<char*>(&version), sizeof(version));
    
    // Write database metadata
    uint64_t timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    file.write(reinterpret_cast<char*>(&timestamp), sizeof(timestamp));
    
    // TODO: Serialize database content
    // This would write all tables, columns, and data
    // For now, we write a simple placeholder
    
    file.close();
}

void DatabaseFile::read_from_file() {
    std::ifstream file(filepath_, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file for reading: " + filepath_);
    }
    
    // Read and verify header
    uint32_t magic = 0;
    uint32_t version = 0;
    
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    
    if (magic != DB_MAGIC) {
        throw std::runtime_error("Invalid database file format");
    }
    
    if (version != DB_VERSION) {
        throw std::runtime_error("Incompatible database version");
    }
    
    // Read metadata
    uint64_t timestamp = 0;
    file.read(reinterpret_cast<char*>(&timestamp), sizeof(timestamp));
    
    // TODO: Deserialize database content
    // This would read all tables, columns, and data
    
    file.close();
}

} // namespace lyradb
