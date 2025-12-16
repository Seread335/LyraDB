#pragma once

#include <string>
#include <vector>
#include <memory>
#include "table_format.h"
#include "column_serializer.h"
#include "schema.h"
#include "compression.h"

namespace lyradb {

// Forward declarations
class Table;
namespace storage {

// Forward declarations
class ColumnWriter;
class ColumnReader;

/**
 * @brief Writes a complete multi-column table to disk
 * 
 * Manages coordination between multiple ColumnWriter instances
 * and writes table-level manifest and metadata.
 */
class TableWriter {
public:
    /**
     * @brief Initialize table writer
     * @param filepath Path to table file (.lyta)
     * @param schema Table schema
     * @param base_path Base directory for column files (.lycol)
     */
    TableWriter(const std::string& filepath,
                const Schema& schema,
                const std::string& base_path = ".");

    ~TableWriter();

    /**
     * @brief Write all pages for a column
     * @param column_id Column index in schema
     * @param pages Vector of page data (each page is vector<uint8_t>)
     * @param row_count Total rows in table
     * @param compression_type Algorithm to use for this column
     * @param stats Compression statistics from ColumnWriter
     */
    void write_column_pages(
        uint32_t column_id,
        const std::vector<std::vector<uint8_t>>& pages,
        uint64_t row_count,
        uint8_t compression_type);

    /**
     * @brief Finalize table write
     * 
     * Closes all column files, calculates statistics, and writes
     * table manifest to main table file.
     */
    void finalize();

    /**
     * @brief Get current table statistics
     * @return TableStatistics struct with aggregated metrics
     */
    const TableStatistics& get_statistics() const;

    /**
     * @brief Check if table is finalized
     * @return true if finalize() has been called
     */
    bool is_finalized() const;

private:
    std::string filepath_;                              // Path to .lyta file
    std::string base_path_;                             // Base directory for .lycol files
    Schema schema_;
    std::vector<std::unique_ptr<ColumnWriter>> writers_;
    TableStatistics statistics_;
    uint64_t total_rows_;
    bool finalized_;
    std::vector<TableColumnMetadata> column_metadata_;

    // Helper methods
    void initialize_column_writers();
    void write_table_manifest();
    std::string get_column_filepath(uint32_t column_id) const;
};

/**
 * @brief Reads a complete multi-column table from disk
 * 
 * Loads table manifest and coordinates multiple ColumnReader instances
 * for efficient random access to table data.
 */
class TableReader {
public:
    /**
     * @brief Initialize table reader
     * @param filepath Path to table file (.lyta)
     */
    explicit TableReader(const std::string& filepath);

    ~TableReader();

    /**
     * @brief Get table schema
     * @return Reference to table schema
     */
    const Schema& get_schema() const;

    /**
     * @brief Read all pages for a column
     * @param column_id Column index in schema
     * @return Vector of pages (each page is vector<uint8_t>)
     */
    std::vector<std::vector<uint8_t>> read_column_pages(
        uint32_t column_id);

    /**
     * @brief Read rows by range
     * @param start_row Starting row index (0-based)
     * @param num_rows Number of rows to read
     * @return Table object with selected rows
     */
    std::shared_ptr<Table> read_rows(uint64_t start_row, uint64_t num_rows);

    /**
     * @brief Read specific row
     * @param row_id Row index (0-based)
     * @return Row data as vector<uint8_t>
     */
    std::vector<uint8_t> read_row(uint64_t row_id);

    /**
     * @brief Get table statistics
     * @return Reference to cached table statistics
     */
    const TableStatistics& get_statistics() const;

    /**
     * @brief Validate table integrity
     * 
     * Verifies checksums and validates all columns.
     * @return true if all checks pass
     */
    bool validate();

    /**
     * @brief Get manifest for current table
     * @return Reference to table manifest
     */
    const TableManifest& get_manifest() const;

    /**
     * @brief Get total row count
     * @return Number of rows in table
     */
    uint64_t get_row_count() const;

    /**
     * @brief Get column count
     * @return Number of columns in table
     */
    uint32_t get_column_count() const;

private:
    std::string filepath_;
    Schema schema_;
    std::vector<std::unique_ptr<ColumnReader>> readers_;
    TableManifest manifest_;
    TableStatistics statistics_;
    bool loaded_;

    // Helper methods
    void load_table_manifest();
    void initialize_column_readers();
    std::string get_column_filepath(uint32_t column_id) const;
};

}  // namespace storage
}  // namespace lyradb
