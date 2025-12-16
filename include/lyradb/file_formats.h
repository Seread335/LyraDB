#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <cstdint>

namespace lyradb {
namespace formats {

// ============================================================================
// FILE FORMAT SPECIFICATIONS
// ============================================================================

/**
 * .LYRADB - Full Database Format
 * ==============================
 * Purpose: Complete database with all features
 * Features:
 *   - Full compression (ZSTD, RLE, Dictionary)
 *   - B-tree and hash indexes
 *   - Advanced query optimization
 *   - Transaction support
 *   - Full schema with all data types
 * 
 * Structure:
 *   [HEADER]
 *   - Magic: "LYRADB" (6 bytes)
 *   - Version: 1.2.0 (4 bytes)
 *   - Flags: Compression, Encryption, Indexes (2 bytes)
 *   - Timestamp (8 bytes)
 *   
 *   [METADATA SECTION]
 *   - Schema info
 *   - Index definitions
 *   - Statistics
 *   
 *   [DATA SECTION]
 *   - Compressed columns
 *   - Index structures
 *   
 *   [FOOTER]
 *   - Checksum
 *   - EOF marker
 */
struct LyraDBFormat {
    static constexpr const char* MAGIC = "LYRADB";
    static constexpr uint32_t VERSION = 0x010200; // 1.2.0
    static constexpr uint32_t HEADER_SIZE = 20;
    
    // Flags
    static constexpr uint8_t FLAG_COMPRESSION = 0x01;
    static constexpr uint8_t FLAG_ENCRYPTION = 0x02;
    static constexpr uint8_t FLAG_INDEXES = 0x04;
    static constexpr uint8_t FLAG_TRANSACTIONS = 0x08;
};

/**
 * .LYRADBITE - Lightweight Embedded Format
 * ========================================
 * Purpose: Compact database for embedded systems & mobile
 * Features:
 *   - Minimal compression (RLE only)
 *   - Single hash index per table
 *   - Basic query optimization
 *   - No transactions (simple append-only)
 *   - Reduced schema complexity
 * 
 * Structure:
 *   [HEADER]
 *   - Magic: "LYRADBITE" (9 bytes)
 *   - Version: 1.0 (2 bytes)
 *   - Flags: Minimal (1 byte)
 *   
 *   [METADATA SECTION]
 *   - Simplified schema
 *   - Single index definition
 *   
 *   [DATA SECTION]
 *   - Minimally compressed columns
 *   
 *   [FOOTER]
 *   - Simple checksum
 */
struct LyraDBiteFormat {
    static constexpr const char* MAGIC = "LYRADBITE";
    static constexpr uint32_t VERSION = 0x010000; // 1.0.0
    static constexpr uint32_t HEADER_SIZE = 12;
    
    // Flags (minimal)
    static constexpr uint8_t FLAG_RLE_COMPRESSION = 0x01;
    static constexpr uint8_t FLAG_SINGLE_INDEX = 0x02;
};

/**
 * .LYRA - Ultra-Lightweight CSV-Like Format
 * ========================================
 * Purpose: Simple text-based format for data exchange & portability
 * Features:
 *   - No compression (text-based)
 *   - No indexes
 *   - No optimization
 *   - Human-readable
 *   - Easy to parse/import
 * 
 * Structure:
 *   [HEADER LINE]
 *   # LYRA 1.0
 *   # Schema: column1:type, column2:type, ...
 *   # Rows: N
 *   
 *   [DATA LINES]
 *   value1,value2,value3,...
 *   value1,value2,value3,...
 *   
 *   [FOOTER]
 *   # EOF
 */
struct LyraFormat {
    static constexpr const char* MAGIC = "# LYRA";
    static constexpr const char* VERSION = "1.0";
};

// ============================================================================
// FILE FORMAT HANDLER INTERFACE
// ============================================================================

class FileFormatHandler {
public:
    virtual ~FileFormatHandler() = default;
    
    // File operations
    virtual bool write_database(const std::string& filename) = 0;
    virtual bool read_database(const std::string& filename) = 0;
    virtual bool validate_format(const std::string& filename) = 0;
    
    // Metadata
    virtual std::string get_format_name() const = 0;
    virtual std::string get_file_extension() const = 0;
    virtual uint32_t get_version() const = 0;
};

// ============================================================================
// LYRADB FORMAT HANDLER
// ============================================================================

class LyraDBFormatHandler : public FileFormatHandler {
private:
    struct Header {
        char magic[7];           // "LYRADB"
        uint32_t version;
        uint8_t flags;
        uint64_t timestamp;
        uint32_t checksum;
    };
    
public:
    bool write_database(const std::string& filename) override;
    bool read_database(const std::string& filename) override;
    bool validate_format(const std::string& filename) override;
    
    std::string get_format_name() const override { return "LyraDB Full Format"; }
    std::string get_file_extension() const override { return ".lyradb"; }
    uint32_t get_version() const override { return LyraDBFormat::VERSION; }
    
private:
    bool write_header(std::ofstream& file);
    bool read_header(std::ifstream& file);
    bool write_metadata(std::ofstream& file);
    bool read_metadata(std::ifstream& file);
    bool write_data(std::ofstream& file);
    bool read_data(std::ifstream& file);
};

// ============================================================================
// LYRADBITE FORMAT HANDLER
// ============================================================================

class LyraDBiteFormatHandler : public FileFormatHandler {
private:
    struct Header {
        char magic[10];          // "LYRADBITE"
        uint32_t version;
        uint8_t flags;
        uint32_t checksum;
    };
    
public:
    bool write_database(const std::string& filename) override;
    bool read_database(const std::string& filename) override;
    bool validate_format(const std::string& filename) override;
    
    std::string get_format_name() const override { return "LyraDB Embedded Format"; }
    std::string get_file_extension() const override { return ".lyradbite"; }
    uint32_t get_version() const override { return LyraDBiteFormat::VERSION; }
    
private:
    bool write_header(std::ofstream& file);
    bool read_header(std::ifstream& file);
    bool compress_for_embedded();
    bool decompress_from_embedded();
};

// ============================================================================
// LYRA FORMAT HANDLER
// ============================================================================

class LyraFormatHandler : public FileFormatHandler {
public:
    bool write_database(const std::string& filename) override;
    bool read_database(const std::string& filename) override;
    bool validate_format(const std::string& filename) override;
    
    std::string get_format_name() const override { return "Lyra Text Format"; }
    std::string get_file_extension() const override { return ".lyra"; }
    uint32_t get_version() const override { return 0x010000; }  // 1.0.0
    
private:
    bool write_csv_header(std::ofstream& file);
    bool write_csv_data(std::ofstream& file);
    bool parse_csv_line(const std::string& line, std::vector<std::string>& fields);
};

// ============================================================================
// FORMAT REGISTRY & FACTORY
// ============================================================================

class FileFormatRegistry {
private:
    static std::map<std::string, std::unique_ptr<FileFormatHandler>> handlers;
    
public:
    static void register_handlers();
    static FileFormatHandler* get_handler(const std::string& extension);
    static std::vector<std::string> get_supported_formats();
    static std::string detect_format(const std::string& filename);
};

// ============================================================================
// DISTRIBUTION UTILITIES
// ============================================================================

class DistributionPackager {
public:
    /**
     * Create distribution package with all 3 formats
     * Useful for:
     *   - Data migration
     *   - Multi-platform support
     *   - Backup strategies
     */
    static bool create_distribution_package(
        const std::string& database_file,
        const std::string& output_dir);
    
    /**
     * Convert between formats
     * Example: Convert .lyradb → .lyra for export
     */
    static bool convert_format(
        const std::string& input_file,
        const std::string& input_format,
        const std::string& output_file,
        const std::string& output_format);
    
    /**
     * Merge multiple format files into single database
     */
    static bool merge_formats(
        const std::vector<std::string>& input_files,
        const std::string& output_file,
        const std::string& output_format);
};

}  // namespace formats
}  // namespace lyradb
