#include "lyradb/table_serializer.h"
#include "lyradb/table_format.h"
#include "lyradb/schema.h"
#include "lyradb/storage_format.h"
#include "lyradb/compression.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <stdexcept>

namespace lyradb {
namespace storage {

// ============================================================================
// TableWriter Implementation
// ============================================================================

TableWriter::TableWriter(const std::string& filepath,
                         const Schema& schema,
                         const std::string& base_path)
    : filepath_(filepath),
      base_path_(base_path),
      schema_(schema),
      total_rows_(0),
      finalized_(false) {
    
    initialize_column_writers();
    
    // Initialize statistics
    statistics_.table_name = "default";
    statistics_.total_columns = schema.num_columns();
    statistics_.table_version = 1;
    statistics_.timestamp_created = 
        std::chrono::system_clock::now().time_since_epoch().count();
    statistics_.column_stats.resize(schema.num_columns());
}

TableWriter::~TableWriter() {
    // Ensure finalization is called
    if (!finalized_) {
        try {
            finalize();
        } catch (...) {
            // Suppress exceptions in destructor
        }
    }
}

void TableWriter::initialize_column_writers() {
    writers_.clear();
    
    for (uint32_t i = 0; i < schema_.num_columns(); ++i) {
        std::string col_filepath = get_column_filepath(i);
        const auto& col_def = schema_.get_column(i);
        uint8_t data_type = static_cast<uint8_t>(col_def.type);
        auto writer = std::make_unique<ColumnWriter>(col_filepath, i, data_type);
        writers_.push_back(std::move(writer));
    }
}

    std::string TableWriter::get_column_filepath(uint32_t column_id) const {
    std::ostringstream oss;
    oss << base_path_ << "/column_" << column_id << ".lycol";
    return oss.str();
}

void TableWriter::write_column_pages(
    uint32_t column_id,
    const std::vector<std::vector<uint8_t>>& pages,
    uint64_t row_count,
    uint8_t compression_type) {
    
    if (finalized_) {
        throw std::runtime_error("Cannot write to finalized table");
    }
    
    if (column_id >= writers_.size()) {
        throw std::out_of_range("Invalid column ID");
    }
    
    // Update total row count
    total_rows_ = std::max(total_rows_, row_count);
    
    // Write pages using column writer
    // Calculate rows per page (assume equal distribution for now)
    uint32_t rows_per_page = pages.empty() ? 0 : (row_count + pages.size() - 1) / pages.size();
    
    for (const auto& page : pages) {
        writers_[column_id]->write_page(page.data(), page.size(), 
                                       rows_per_page, compression_type);
    }
    
    // Update column metadata
    TableColumnMetadata meta;
    meta.column_id = column_id;
    meta.column_file_offset = 0;  // Will be computed at finalization
    meta.column_file_size = 0;    // Will be computed at finalization
    meta.compression_algorithm = compression_type;
    meta.page_count = pages.size();
    meta.compression_ratio = 100;  // Placeholder
    meta.checksum = 0;
    
    // Store metadata (resize if needed)
    if (column_metadata_.size() <= column_id) {
        column_metadata_.resize(column_id + 1);
    }
    column_metadata_[column_id] = meta;
    
    // Update statistics for this column
    if (column_id < statistics_.column_stats.size()) {
        auto& col_stat = statistics_.column_stats[column_id];
        col_stat.column_id = column_id;
        col_stat.page_count = pages.size();
        col_stat.compression_ratio = 100;  // Placeholder
        col_stat.uncompressed_bytes = row_count * 8;  // Placeholder
        col_stat.compressed_bytes = 0;  // Placeholder
    }
}

void TableWriter::finalize() {
    if (finalized_) {
        return;
    }
    
    // Close all column writers
    for (auto& writer : writers_) {
        if (writer) {
            // Finalize each column
        }
    }
    
    // Calculate final statistics
    statistics_.total_rows = total_rows_;
    statistics_.uncompressed_bytes = 0;
    statistics_.compressed_bytes = 0;
    
    double total_ratio = 0.0;
    for (const auto& col_stat : statistics_.column_stats) {
        statistics_.uncompressed_bytes += col_stat.uncompressed_bytes;
        statistics_.compressed_bytes += col_stat.compressed_bytes;
        total_ratio += col_stat.compression_ratio;
    }
    
    if (statistics_.total_columns > 0) {
        statistics_.overall_compression_ratio = 
            total_ratio / statistics_.total_columns;
    }
    
    // Write table manifest
    write_table_manifest();
    
    finalized_ = true;
}

void TableWriter::write_table_manifest() {
    // Create table header
    TableFileHeader header;
    header.magic = LYTA_MAGIC;
    header.version = LYTA_VERSION;
    header.row_count = total_rows_;
    header.column_count = schema_.num_columns();
    header.schema_id = 1;
    header.checksum = 0;
    
    // Open manifest file
    std::ofstream manifest_file(filepath_, std::ios::binary);
    if (!manifest_file.is_open()) {
        throw std::runtime_error("Failed to open table manifest file");
    }
    
    // Serialize and write header
    auto header_bytes = format_utils::serialize_table_header(header);
    
    // Update checksum
    uint32_t checksum = format_utils::calculate_table_checksum(
        header_bytes.data(), header_bytes.size());
    header.checksum = checksum;
    header_bytes = format_utils::serialize_table_header(header);
    
    manifest_file.write(reinterpret_cast<const char*>(header_bytes.data()),
                       header_bytes.size());
    
    // Write column metadata
    for (uint32_t i = 0; i < column_metadata_.size(); ++i) {
        auto meta_bytes = format_utils::serialize_column_metadata(
            column_metadata_[i]);
        
        // Update checksum
        uint32_t col_checksum = format_utils::calculate_table_checksum(
            meta_bytes.data(), meta_bytes.size());
        column_metadata_[i].checksum = col_checksum;
        meta_bytes = format_utils::serialize_column_metadata(
            column_metadata_[i]);
        
        manifest_file.write(reinterpret_cast<const char*>(meta_bytes.data()),
                           meta_bytes.size());
    }
    
    // Write statistics
    auto stats_bytes = format_utils::serialize_table_statistics(statistics_);
    manifest_file.write(reinterpret_cast<const char*>(stats_bytes.data()),
                       stats_bytes.size());
    
    manifest_file.close();
}

const TableStatistics& TableWriter::get_statistics() const {
    return statistics_;
}

bool TableWriter::is_finalized() const {
    return finalized_;
}

// ============================================================================
// TableReader Implementation
// ============================================================================

TableReader::TableReader(const std::string& filepath)
    : filepath_(filepath),
      loaded_(false) {
    
    load_table_manifest();
    initialize_column_readers();
}

TableReader::~TableReader() = default;

void TableReader::load_table_manifest() {
    // Open and read manifest file
    std::ifstream manifest_file(filepath_, std::ios::binary);
    if (!manifest_file.is_open()) {
        throw std::runtime_error("Failed to open table manifest file");
    }
    
    // Read header
    std::vector<uint8_t> header_buffer(sizeof(TableFileHeader));
    manifest_file.read(reinterpret_cast<char*>(header_buffer.data()),
                      header_buffer.size());
    
    if (manifest_file.gcount() != static_cast<std::streamsize>(header_buffer.size())) {
        throw std::runtime_error("Failed to read complete table header");
    }
    
    // Deserialize header
    manifest_.header = format_utils::deserialize_table_header(
        header_buffer.data(), header_buffer.size());
    
    // Read column metadata
    manifest_.column_metadata.resize(manifest_.header.column_count);
    for (uint32_t i = 0; i < manifest_.header.column_count; ++i) {
        std::vector<uint8_t> meta_buffer(sizeof(TableColumnMetadata));
        manifest_file.read(reinterpret_cast<char*>(meta_buffer.data()),
                          meta_buffer.size());
        
        if (manifest_file.gcount() != static_cast<std::streamsize>(meta_buffer.size())) {
            throw std::runtime_error("Failed to read complete column metadata");
        }
        
        manifest_.column_metadata[i] = 
            format_utils::deserialize_column_metadata(
                meta_buffer.data(), meta_buffer.size());
    }
    
    // Read statistics
    std::vector<uint8_t> stats_buffer(4096);  // Initial buffer size
    manifest_file.read(reinterpret_cast<char*>(stats_buffer.data()),
                      stats_buffer.size());
    
    size_t stats_size = manifest_file.gcount();
    if (stats_size > 0) {
        stats_buffer.resize(stats_size);
        manifest_.statistics = format_utils::deserialize_table_statistics(
            stats_buffer.data(), stats_buffer.size());
        statistics_ = manifest_.statistics;
    }
    
    manifest_.valid = true;
    loaded_ = true;
    manifest_file.close();
}

void TableReader::initialize_column_readers() {
    readers_.clear();
    readers_.resize(manifest_.header.column_count);
    
    for (uint32_t i = 0; i < manifest_.header.column_count; ++i) {
        std::ostringstream oss;
        oss << "." << "/column_" << i << ".lycol";
        std::string col_filepath = oss.str();
        
        try {
            auto reader = std::make_unique<ColumnReader>(col_filepath);
            readers_[i] = std::move(reader);
        } catch (const std::exception& e) {
            throw std::runtime_error(
                std::string("Failed to initialize column reader: ") + e.what());
        }
    }
}

const Schema& TableReader::get_schema() const {
    return schema_;
}

std::vector<std::vector<uint8_t>> TableReader::read_column_pages(
    uint32_t column_id) {
    
    if (column_id >= readers_.size()) {
        throw std::out_of_range("Invalid column ID");
    }
    
    std::vector<std::vector<uint8_t>> pages;
    
    if (!readers_[column_id]) {
        throw std::runtime_error("Column reader not initialized");
    }
    
    // Read all pages for this column
    uint32_t page_count = manifest_.column_metadata[column_id].page_count;
    for (uint32_t i = 0; i < page_count; ++i) {
        auto page_data = readers_[column_id]->read_page(i);
        pages.push_back(page_data);
    }
    
    return pages;
}

std::shared_ptr<Table> TableReader::read_rows(uint64_t start_row, uint64_t num_rows) {
    // TODO: Implement row range reading from multiple columns
    // For now, return nullptr as placeholder
    return nullptr;
}

std::vector<uint8_t> TableReader::read_row(uint64_t row_id) {
    if (row_id >= manifest_.header.row_count) {
        throw std::out_of_range("Row ID out of range");
    }
    
    std::vector<uint8_t> row_data;
    
    // Read row data from all columns (stub)
    for (uint32_t col_id = 0; col_id < manifest_.header.column_count; ++col_id) {
        // Read this column's data for this row
    }
    
    return row_data;
}

const TableStatistics& TableReader::get_statistics() const {
    return statistics_;
}

bool TableReader::validate() {
    // Validate header checksum
    if (!format_utils::verify_table_header_checksum(manifest_.header)) {
        return false;
    }
    
    // Validate all column metadata
    for (const auto& meta : manifest_.column_metadata) {
        if (!format_utils::verify_column_metadata_checksum(meta)) {
            return false;
        }
    }
    
    // Validate column readers
    for (auto& reader : readers_) {
        if (reader && !reader->validate()) {
            return false;
        }
    }
    
    return true;
}

const TableManifest& TableReader::get_manifest() const {
    return manifest_;
}

uint64_t TableReader::get_row_count() const {
    return manifest_.header.row_count;
}

uint32_t TableReader::get_column_count() const {
    return manifest_.header.column_count;
}

}  // namespace storage
}  // namespace lyradb
