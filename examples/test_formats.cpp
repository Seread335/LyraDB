/*
 * LyraDB Formats Library - Test and Example
 * Demonstrates how to use the .lyradb, .lyradbite, and .lyra file formats
 * 
 * Compile with: g++ -std=c++17 -I../include test_formats.cpp ../src/formats/lyradb_formats.cpp -o test_formats
 */

#include <iostream>
#include <iomanip>
#include "lyradb/lyradb_formats.h"

using namespace lyradb;

// ============================================================================
// Example 1: Creating and Writing a .lyradb Database File
// ============================================================================

void Example_CreateDatabaseFile() {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "EXAMPLE 1: Creating a .lyradb Database File" << std::endl;
    std::cout << std::string(80, '=') << "\n" << std::endl;
    
    // Create a database format object
    LyraDBFormat* database = LyraFileFormatManager::CreateDatabaseFormat();
    
    // Set database metadata
    database->SetDatabaseName("MyStore");
    database->SetTotalTables(3);
    database->SetTotalRows(15000);
    database->SetDataSize("50.5 MB");
    database->SetCompressedSize("12.3 MB");
    database->SetCompressionRatio(0.76);
    database->SetRecoveryLogEnabled(true);
    database->SetSchemaHash("8f3a4b2c1e9d7f5a");
    database->SetEncryptionStatus("DISABLED");
    
    // Add tables
    LyraDBFormat::TableInfo table1;
    table1.name = "customers";
    table1.rowCount = 5000;
    table1.sizeKB = 2048;
    database->AddTable(table1);
    
    LyraDBFormat::TableInfo table2;
    table2.name = "orders";
    table2.rowCount = 8000;
    table2.sizeKB = 3072;
    database->AddTable(table2);
    
    LyraDBFormat::TableInfo table3;
    table3.name = "products";
    table3.rowCount = 2000;
    table3.sizeKB = 1024;
    database->AddTable(table3);
    
    // Add indexes
    LyraDBFormat::IndexInfo idx1;
    idx1.name = "idx_customer_id";
    idx1.type = "B-Tree";
    idx1.tableName = "customers";
    idx1.columnName = "customer_id";
    database->AddIndex(idx1);
    
    LyraDBFormat::IndexInfo idx2;
    idx2.name = "idx_order_date";
    idx2.type = "B-Tree";
    idx2.tableName = "orders";
    idx2.columnName = "order_date";
    database->AddIndex(idx2);
    
    // Set compression statistics
    LyraDBFormat::CompressionStats stats;
    stats.rleRatio = 0.95;
    stats.deltaRatio = 0.88;
    stats.dictionaryRatio = 0.72;
    stats.bitPackingRatio = 0.65;
    stats.zstdRatio = 0.55;
    stats.selected = "ZSTD";
    database->SetCompressionStats(stats);
    
    // Print and save
    std::cout << database->ToString() << std::endl;
    
    // Write to file
    std::string filename = "example_store.lyradb";
    if (database->WriteToFile(filename)) {
        std::cout << "\n✓ Successfully created: " << filename << std::endl;
        std::cout << "  File size: " << LyraFileFormatManager::GetFileSize(filename) 
                  << " bytes\n" << std::endl;
    } else {
        std::cerr << "✗ Failed to create database file\n" << std::endl;
    }
    
    delete database;
}

// ============================================================================
// Example 2: Creating and Writing a .lyradbite Iterator File
// ============================================================================

void Example_CreateIteratorFile() {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "EXAMPLE 2: Creating a .lyradbite Iterator File" << std::endl;
    std::cout << std::string(80, '=') << "\n" << std::endl;
    
    // Create an iterator format object
    LyraDBIteratorFormat* iterator = LyraFileFormatManager::CreateIteratorFormat();
    
    // Set iterator metadata
    iterator->SetIteratorName("customers_full_scan");
    iterator->SetSourceDatabase("MyStore");
    iterator->SetSourceTable("customers");
    iterator->SetRowCount(5000);
    iterator->SetPageSize(4096);
    
    // Configure iteration
    LyraDBIteratorFormat::IterationConfig config;
    config.bufferSize = 4096;
    config.cachingEnabled = true;
    config.prefetchSize = 2048;
    config.batchSize = 1000;
    config.compression = "ZSTD";
    iterator->SetIterationConfig(config);
    
    // Set cursor information
    LyraDBIteratorFormat::CursorInfo cursor;
    cursor.startOffset = 0;
    cursor.endOffset = 5000;
    cursor.currentPosition = 0;
    cursor.direction = "FORWARD";
    cursor.status = "INITIALIZED";
    iterator->SetCursorInfo(cursor);
    
    // Add column mappings
    LyraDBIteratorFormat::ColumnMapping col1;
    col1.name = "customer_id";
    col1.type = "INTEGER";
    col1.size = "8 bytes";
    iterator->AddColumn(col1);
    
    LyraDBIteratorFormat::ColumnMapping col2;
    col2.name = "customer_name";
    col2.type = "VARCHAR(255)";
    col2.size = "255 bytes";
    iterator->AddColumn(col2);
    
    LyraDBIteratorFormat::ColumnMapping col3;
    col3.name = "email";
    col3.type = "VARCHAR(255)";
    col3.size = "255 bytes";
    iterator->AddColumn(col3);
    
    // Set performance statistics
    LyraDBIteratorFormat::PerformanceStats perfStats;
    perfStats.totalPagesRead = 2;
    perfStats.bufferHits = 1975;
    perfStats.bufferMisses = 25;
    perfStats.averageRowSize = "512 bytes";
    perfStats.estimatedIterationTime = "2.5 seconds";
    perfStats.throughputExpected = "2000 rows/sec";
    iterator->SetPerformanceStats(perfStats);
    
    // Set optimization
    iterator->SetIndexUsage("idx_customer_id");
    iterator->EnablePrefetch(true);
    iterator->EnableParallelization(4);
    
    // Print and save
    std::cout << iterator->ToString() << std::endl;
    
    // Write to file
    std::string filename = "example_customers_iter.lyradbite";
    if (iterator->WriteToFile(filename)) {
        std::cout << "\n✓ Successfully created: " << filename << std::endl;
        std::cout << "  File size: " << LyraFileFormatManager::GetFileSize(filename) 
                  << " bytes\n" << std::endl;
    } else {
        std::cerr << "✗ Failed to create iterator file\n" << std::endl;
    }
    
    delete iterator;
}

// ============================================================================
// Example 3: Creating and Writing a .lyra Archive File
// ============================================================================

void Example_CreateArchiveFile() {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "EXAMPLE 3: Creating a .lyra Archive File" << std::endl;
    std::cout << std::string(80, '=') << "\n" << std::endl;
    
    // Create an archive format object
    LyraArchiveFormat* archive = LyraFileFormatManager::CreateArchiveFormat();
    
    // Set archive metadata
    archive->SetArchiveName("MyStore_Backup_2025-12-16");
    archive->SetBackupType("FULL");
    archive->SetSourceSystem("Production-Server-01");
    archive->SetCompressionLevel(9);
    
    // Set database information
    archive->SetDatabaseName("MyStore");
    archive->SetDatabaseVersion("1.0.0");
    archive->SetTablesIncluded(3);
    archive->SetTotalRowsArchived(15000);
    archive->SetUncompressedSize("50.5 MB");
    archive->SetCompressedSize("12.3 MB");
    
    // Add archive entries
    LyraArchiveFormat::ArchiveEntry entry1;
    entry1.filename = "customers.data";
    entry1.description = "Customer table data with 5000 rows";
    entry1.size = "2.048 MB";
    archive->AddEntry(entry1);
    
    LyraArchiveFormat::ArchiveEntry entry2;
    entry2.filename = "orders.data";
    entry2.description = "Order table data with 8000 rows";
    entry2.size = "3.072 MB";
    archive->AddEntry(entry2);
    
    LyraArchiveFormat::ArchiveEntry entry3;
    entry3.filename = "products.data";
    entry3.description = "Product table data with 2000 rows";
    entry3.size = "1.024 MB";
    archive->AddEntry(entry3);
    
    LyraArchiveFormat::ArchiveEntry entry4;
    entry4.filename = "indexes.idx";
    entry4.description = "All table indexes";
    entry4.size = "0.256 MB";
    archive->AddEntry(entry4);
    
    // Set integrity verification
    LyraArchiveFormat::IntegrityVerification integrity;
    integrity.checksumAlgorithm = "CRC64";
    integrity.databaseChecksum = "A1B2C3D4E5F6G7H8";
    integrity.totalEntryCount = 4;
    integrity.integrityStatus = "VERIFIED";
    archive->SetIntegrityVerification(integrity);
    
    // Set backup schedule
    LyraArchiveFormat::BackupSchedule schedule;
    schedule.fullBackupInterval = "Weekly (Sunday 2:00 AM)";
    schedule.incrementalBackupInterval = "Daily (2:00 AM)";
    schedule.lastFullBackup = "2025-12-14";
    schedule.nextFullBackup = "2025-12-21";
    schedule.retentionDays = 90;
    archive->SetBackupSchedule(schedule);
    
    // Set encryption information
    LyraArchiveFormat::EncryptionInfo encryption;
    encryption.encryptionMethod = "AES-256-GCM";
    encryption.status = "AVAILABLE";
    encryption.keyDerivation = "PBKDF2";
    encryption.iterationCount = 100000;
    archive->SetEncryptionInfo(encryption);
    
    // Set versioning
    archive->SetSchemaVersion("1.0");
    archive->SetDataFormatVersion("1.0");
    archive->SetArchiveFormatVersion("1.0");
    
    // Print and save
    std::cout << archive->ToString() << std::endl;
    
    // Write to file
    std::string filename = "example_backup_2025-12-16.lyra";
    if (archive->WriteToFile(filename)) {
        std::cout << "\n✓ Successfully created: " << filename << std::endl;
        std::cout << "  File size: " << LyraFileFormatManager::GetFileSize(filename) 
                  << " bytes\n" << std::endl;
    } else {
        std::cerr << "✗ Failed to create archive file\n" << std::endl;
    }
    
    delete archive;
}

// ============================================================================
// Example 4: Reading Files and Detecting Format Types
// ============================================================================

void Example_ReadAndDetectFormats() {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "EXAMPLE 4: Reading and Detecting File Formats" << std::endl;
    std::cout << std::string(80, '=') << "\n" << std::endl;
    
    // Test reading database file
    std::cout << "Reading Database File:\n";
    {
        LyraDBFormat* db = LyraFileFormatManager::CreateDatabaseFormat();
        if (db->ReadFromFile("example_store.lyradb")) {
            std::cout << "✓ Successfully read: example_store.lyradb\n";
            std::cout << "  Format Type: " << db->GetFormatType() << "\n";
            std::cout << "  Database: " << db->GetDatabaseName() << "\n";
            std::cout << "  Tables: " << db->GetTotalTables() << "\n";
            std::cout << "  Rows: " << db->GetTotalRows() << "\n\n";
        }
        delete db;
    }
    
    // Test reading iterator file
    std::cout << "Reading Iterator File:\n";
    {
        LyraDBIteratorFormat* iter = LyraFileFormatManager::CreateIteratorFormat();
        if (iter->ReadFromFile("example_customers_iter.lyradbite")) {
            std::cout << "✓ Successfully read: example_customers_iter.lyradbite\n";
            std::cout << "  Format Type: " << iter->GetFormatType() << "\n";
            std::cout << "  Columns: " << iter->GetColumns().size() << "\n\n";
        }
        delete iter;
    }
    
    // Test reading archive file
    std::cout << "Reading Archive File:\n";
    {
        LyraArchiveFormat* arc = LyraFileFormatManager::CreateArchiveFormat();
        if (arc->ReadFromFile("example_backup_2025-12-16.lyra")) {
            std::cout << "✓ Successfully read: example_backup_2025-12-16.lyra\n";
            std::cout << "  Format Type: " << arc->GetFormatType() << "\n";
            std::cout << "  Entries: " << arc->GetTotalEntryCount() << "\n\n";
        }
        delete arc;
    }
    
    // Detect format types
    std::cout << "Format Type Detection:\n";
    std::cout << "  example_store.lyradb -> " 
              << LyraFileFormatManager::DetectFormatType("example_store.lyradb") << "\n";
    std::cout << "  example_customers_iter.lyradbite -> " 
              << LyraFileFormatManager::DetectFormatType("example_customers_iter.lyradbite") << "\n";
    std::cout << "  example_backup_2025-12-16.lyra -> " 
              << LyraFileFormatManager::DetectFormatType("example_backup_2025-12-16.lyra") << "\n\n";
}

// ============================================================================
// Main Function
// ============================================================================

int main() {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "LyraDB Formats Library - Complete Examples" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
    try {
        // Run all examples
        Example_CreateDatabaseFile();
        Example_CreateIteratorFile();
        Example_CreateArchiveFile();
        Example_ReadAndDetectFormats();
        
        // Summary
        std::cout << "\n" << std::string(80, '=') << std::endl;
        std::cout << "All Examples Completed Successfully!" << std::endl;
        std::cout << std::string(80, '=') << std::endl;
        std::cout << "\nGenerated Files:\n";
        std::cout << "  1. example_store.lyradb              (Database Format)\n";
        std::cout << "  2. example_customers_iter.lyradbite  (Iterator Format)\n";
        std::cout << "  3. example_backup_2025-12-16.lyra    (Archive Format)\n";
        std::cout << "\nLibrary Features:\n";
        std::cout << "  ✓ Create .lyradb database files with metadata, tables, and indexes\n";
        std::cout << "  ✓ Create .lyradbite iterator files with cursor tracking\n";
        std::cout << "  ✓ Create .lyra archive files with backup information\n";
        std::cout << "  ✓ Read and validate file formats\n";
        std::cout << "  ✓ Detect file format types automatically\n";
        std::cout << "  ✓ Calculate CRC64 checksums for data integrity\n";
        std::cout << "  ✓ Support compression configuration\n";
        std::cout << "  ✓ Support encryption information\n";
        std::cout << "\n" << std::string(80, '=') << "\n" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
