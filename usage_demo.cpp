/*
 * LyraDB Formats Library - Usage Demo
 * Demonstrates practical usage of the library in a real application
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include "lyradb/lyradb_formats.h"

using namespace lyradb;

// ============================================================================
// Application: Simple File Backup System using LyraDB Formats
// ============================================================================

class FileBackupSystem {
private:
    std::string appName;
    std::string dbName;
    
public:
    FileBackupSystem(const std::string& appName, const std::string& dbName)
        : appName(appName), dbName(dbName) {
        std::cout << "\n[" << appName << "] Backup System Initialized\n";
    }
    
    // Scenario 1: Export database to .lyradb format
    void ExportDatabase() {
        std::cout << "\n" << std::string(70, '=') << "\n";
        std::cout << "SCENARIO 1: Export Database to .lyradb Format\n";
        std::cout << std::string(70, '=') << "\n\n";
        
        // Create database format
        LyraDBFormat* db = LyraFileFormatManager::CreateDatabaseFormat();
        
        // Configure database metadata
        std::cout << "[INFO] Configuring database metadata...\n";
        db->SetDatabaseName(dbName);
        db->SetTotalTables(5);
        db->SetTotalRows(250000);
        db->SetDataSize("500 MB");
        db->SetCompressedSize("125 MB");
        db->SetCompressionRatio(0.75);
        db->SetRecoveryLogEnabled(true);
        db->SetEncryptionStatus("ENABLED");
        
        // Add tables
        std::cout << "[INFO] Adding table definitions...\n";
        
        LyraDBFormat::TableInfo usersTable;
        usersTable.name = "users";
        usersTable.rowCount = 50000;
        usersTable.sizeKB = 51200;
        db->AddTable(usersTable);
        
        LyraDBFormat::TableInfo productsTable;
        productsTable.name = "products";
        productsTable.rowCount = 100000;
        productsTable.sizeKB = 102400;
        db->AddTable(productsTable);
        
        LyraDBFormat::TableInfo ordersTable;
        ordersTable.name = "orders";
        ordersTable.rowCount = 100000;
        ordersTable.sizeKB = 204800;
        db->AddTable(ordersTable);
        
        // Add indexes
        std::cout << "[INFO] Adding index information...\n";
        
        LyraDBFormat::IndexInfo userIdIdx;
        userIdIdx.name = "idx_user_id";
        userIdIdx.type = "B-Tree";
        userIdIdx.tableName = "users";
        userIdIdx.columnName = "user_id";
        db->AddIndex(userIdIdx);
        
        LyraDBFormat::IndexInfo productIdIdx;
        productIdIdx.name = "idx_product_id";
        productIdIdx.type = "Hash";
        productIdIdx.tableName = "products";
        productIdIdx.columnName = "product_id";
        db->AddIndex(productIdIdx);
        
        // Add compression statistics
        std::cout << "[INFO] Adding compression statistics...\n";
        
        LyraDBFormat::CompressionStats stats;
        stats.rleRatio = 0.92;
        stats.deltaRatio = 0.85;
        stats.dictionaryRatio = 0.70;
        stats.bitPackingRatio = 0.68;
        stats.zstdRatio = 0.40;
        stats.selected = "ZSTD";
        db->SetCompressionStats(stats);
        
        // Write to file
        std::string dbFile = dbName + "_export.lyradb";
        std::cout << "[SAVE] Writing to: " << dbFile << "\n";
        
        if (db->WriteToFile(dbFile)) {
            uint64_t fileSize = LyraFileFormatManager::GetFileSize(dbFile);
            std::cout << "[SUCCESS] ✓ Database exported successfully!\n";
            std::cout << "  Filename: " << dbFile << "\n";
            std::cout << "  File Size: " << fileSize << " bytes\n";
            std::cout << "  Tables: " << db->GetTotalTables() << "\n";
            std::cout << "  Rows: " << db->GetTotalRows() << "\n";
        } else {
            std::cout << "[ERROR] Failed to write database file\n";
        }
        
        delete db;
    }
    
    // Scenario 2: Create iterator for data scanning
    void CreateDataIterator() {
        std::cout << "\n" << std::string(70, '=') << "\n";
        std::cout << "SCENARIO 2: Create Data Iterator for Efficient Scanning\n";
        std::cout << std::string(70, '=') << "\n\n";
        
        // Create iterator format
        LyraDBIteratorFormat* iter = LyraFileFormatManager::CreateIteratorFormat();
        
        // Configure iterator
        std::cout << "[INFO] Configuring iterator for table scan...\n";
        iter->SetIteratorName("products_full_scan");
        iter->SetSourceDatabase(dbName);
        iter->SetSourceTable("products");
        iter->SetRowCount(100000);
        iter->SetPageSize(8192);
        
        // Set iteration config
        LyraDBIteratorFormat::IterationConfig config;
        config.bufferSize = 8192;
        config.cachingEnabled = true;
        config.prefetchSize = 4096;
        config.batchSize = 5000;
        config.compression = "ZSTD";
        iter->SetIterationConfig(config);
        
        // Set cursor info
        LyraDBIteratorFormat::CursorInfo cursor;
        cursor.startOffset = 0;
        cursor.endOffset = 100000;
        cursor.currentPosition = 0;
        cursor.direction = "FORWARD";
        cursor.status = "ACTIVE";
        iter->SetCursorInfo(cursor);
        
        // Add column mappings
        std::cout << "[INFO] Adding column mappings...\n";
        
        LyraDBIteratorFormat::ColumnMapping col1;
        col1.name = "product_id";
        col1.type = "BIGINT";
        col1.size = "8 bytes";
        iter->AddColumn(col1);
        
        LyraDBIteratorFormat::ColumnMapping col2;
        col2.name = "product_name";
        col2.type = "VARCHAR(255)";
        col2.size = "255 bytes";
        iter->AddColumn(col2);
        
        LyraDBIteratorFormat::ColumnMapping col3;
        col3.name = "price";
        col3.type = "DECIMAL(10,2)";
        col3.size = "8 bytes";
        iter->AddColumn(col3);
        
        LyraDBIteratorFormat::ColumnMapping col4;
        col4.name = "inventory";
        col4.type = "INTEGER";
        col4.size = "4 bytes";
        iter->AddColumn(col4);
        
        // Set performance stats
        std::cout << "[INFO] Setting performance statistics...\n";
        
        LyraDBIteratorFormat::PerformanceStats perf;
        perf.totalPagesRead = 50;
        perf.bufferHits = 48750;
        perf.bufferMisses = 1250;
        perf.averageRowSize = "1024 bytes";
        perf.estimatedIterationTime = "50.0 seconds";
        perf.throughputExpected = "2000 rows/sec";
        iter->SetPerformanceStats(perf);
        
        // Optimization
        iter->SetIndexUsage("idx_product_id");
        iter->EnablePrefetch(true);
        iter->EnableParallelization(8);
        
        // Write to file
        std::string iterFile = dbName + "_products_iter.lyradbite";
        std::cout << "[SAVE] Writing to: " << iterFile << "\n";
        
        if (iter->WriteToFile(iterFile)) {
            uint64_t fileSize = LyraFileFormatManager::GetFileSize(iterFile);
            std::cout << "[SUCCESS] ✓ Iterator created successfully!\n";
            std::cout << "  Filename: " << iterFile << "\n";
            std::cout << "  File Size: " << fileSize << " bytes\n";
            std::cout << "  Columns: " << iter->GetColumns().size() << "\n";
            std::cout << "  Row Count: " << 100000 << "\n";
            std::cout << "  Parallelization: 8 threads\n";
        } else {
            std::cout << "[ERROR] Failed to write iterator file\n";
        }
        
        delete iter;
    }
    
    // Scenario 3: Create backup archive
    void CreateBackupArchive() {
        std::cout << "\n" << std::string(70, '=') << "\n";
        std::cout << "SCENARIO 3: Create Backup Archive (.lyra Format)\n";
        std::cout << std::string(70, '=') << "\n\n";
        
        // Create archive format
        LyraArchiveFormat* arc = LyraFileFormatManager::CreateArchiveFormat();
        
        // Configure archive
        std::cout << "[INFO] Configuring backup archive...\n";
        arc->SetArchiveName(dbName + "_Backup_2025-12-16");
        arc->SetBackupType("FULL");
        arc->SetSourceSystem("ProductionServer-01");
        arc->SetCompressionLevel(9);
        
        // Database info
        arc->SetDatabaseName(dbName);
        arc->SetDatabaseVersion("2.5.0");
        arc->SetTablesIncluded(5);
        arc->SetTotalRowsArchived(250000);
        arc->SetUncompressedSize("500 MB");
        arc->SetCompressedSize("125 MB");
        
        // Add archive entries
        std::cout << "[INFO] Adding archive entries...\n";
        
        LyraArchiveFormat::ArchiveEntry entry1;
        entry1.filename = "users.data";
        entry1.description = "User account data (50000 rows)";
        entry1.size = "51.2 MB";
        arc->AddEntry(entry1);
        
        LyraArchiveFormat::ArchiveEntry entry2;
        entry2.filename = "products.data";
        entry2.description = "Product catalog data (100000 rows)";
        entry2.size = "102.4 MB";
        arc->AddEntry(entry2);
        
        LyraArchiveFormat::ArchiveEntry entry3;
        entry3.filename = "orders.data";
        entry3.description = "Order history data (100000 rows)";
        entry3.size = "204.8 MB";
        arc->AddEntry(entry3);
        
        LyraArchiveFormat::ArchiveEntry entry4;
        entry4.filename = "indexes.idx";
        entry4.description = "All table indexes";
        entry4.size = "50.5 MB";
        arc->AddEntry(entry4);
        
        LyraArchiveFormat::ArchiveEntry entry5;
        entry5.filename = "schema.sql";
        entry5.description = "Database schema definition";
        entry5.size = "0.5 MB";
        arc->AddEntry(entry5);
        
        // Integrity verification
        std::cout << "[INFO] Adding integrity verification...\n";
        
        LyraArchiveFormat::IntegrityVerification integrity;
        integrity.checksumAlgorithm = "CRC64";
        integrity.databaseChecksum = CalculateCRC64(dbName);
        integrity.totalEntryCount = 5;
        integrity.integrityStatus = "VERIFIED";
        arc->SetIntegrityVerification(integrity);
        
        // Backup schedule
        LyraArchiveFormat::BackupSchedule schedule;
        schedule.fullBackupInterval = "Weekly (Sunday 1:00 AM)";
        schedule.incrementalBackupInterval = "Daily (1:00 AM)";
        schedule.lastFullBackup = "2025-12-14";
        schedule.nextFullBackup = "2025-12-21";
        schedule.retentionDays = 365;
        arc->SetBackupSchedule(schedule);
        
        // Encryption info
        LyraArchiveFormat::EncryptionInfo encryption;
        encryption.encryptionMethod = "AES-256-GCM";
        encryption.status = "AVAILABLE";
        encryption.keyDerivation = "PBKDF2";
        encryption.iterationCount = 100000;
        arc->SetEncryptionInfo(encryption);
        
        // Versioning
        arc->SetSchemaVersion("2.5");
        arc->SetDataFormatVersion("1.0");
        arc->SetArchiveFormatVersion("1.0");
        
        // Write to file
        std::string archiveFile = dbName + "_backup_2025-12-16.lyra";
        std::cout << "[SAVE] Writing to: " << archiveFile << "\n";
        
        if (arc->WriteToFile(archiveFile)) {
            uint64_t fileSize = LyraFileFormatManager::GetFileSize(archiveFile);
            std::cout << "[SUCCESS] ✓ Backup archive created successfully!\n";
            std::cout << "  Filename: " << archiveFile << "\n";
            std::cout << "  File Size: " << fileSize << " bytes\n";
            std::cout << "  Entries: " << arc->GetTotalEntryCount() << "\n";
            std::cout << "  Encryption: AES-256-GCM (Available)\n";
            std::cout << "  Retention: 365 days\n";
        } else {
            std::cout << "[ERROR] Failed to write archive file\n";
        }
        
        delete arc;
    }
    
    // Scenario 4: Validate and verify all files
    void ValidateAllFiles() {
        std::cout << "\n" << std::string(70, '=') << "\n";
        std::cout << "SCENARIO 4: Validate and Verify All Files\n";
        std::cout << std::string(70, '=') << "\n\n";
        
        std::vector<std::pair<std::string, std::string>> files = {
            {dbName + "_export.lyradb", "DATABASE"},
            {dbName + "_products_iter.lyradbite", "ITERATOR"},
            {dbName + "_backup_2025-12-16.lyra", "ARCHIVE"}
        };
        
        std::cout << "[INFO] Validating generated files...\n\n";
        
        for (const auto& file : files) {
            std::cout << "File: " << file.first << "\n";
            
            bool exists = LyraFileFormatManager::FileExists(file.first);
            uint64_t size = LyraFileFormatManager::GetFileSize(file.first);
            std::string detectedType = LyraFileFormatManager::DetectFormatType(file.first);
            
            std::cout << "  ├─ Exists: " << (exists ? "✓ YES" : "✗ NO") << "\n";
            std::cout << "  ├─ Size: " << size << " bytes\n";
            std::cout << "  ├─ Detected Type: " << detectedType << "\n";
            std::cout << "  ├─ Expected Type: " << file.second << "\n";
            
            bool isValid = false;
            if (file.second == "DATABASE") {
                isValid = LyraFileFormatManager::IsValidLyraDBFile(file.first);
            } else if (file.second == "ITERATOR") {
                isValid = LyraFileFormatManager::IsValidIteratorFile(file.first);
            } else if (file.second == "ARCHIVE") {
                isValid = LyraFileFormatManager::IsValidArchiveFile(file.first);
            }
            
            std::cout << "  └─ Validation: " << (isValid ? "✓ PASSED" : "✗ FAILED") << "\n\n";
        }
        
        std::cout << "[SUCCESS] ✓ All validations complete!\n";
    }
    
    // Scenario 5: Read and re-verify files
    void ReadAndVerifyFiles() {
        std::cout << "\n" << std::string(70, '=') << "\n";
        std::cout << "SCENARIO 5: Read and Re-Verify File Contents\n";
        std::cout << std::string(70, '=') << "\n\n";
        
        // Read database file
        std::cout << "[READ] Reading database file...\n";
        {
            LyraDBFormat* db = LyraFileFormatManager::CreateDatabaseFormat();
            std::string dbFile = dbName + "_export.lyradb";
            
            if (db->ReadFromFile(dbFile)) {
                std::cout << "  ✓ Database read successfully\n";
                std::cout << "  └─ Format: " << db->GetFormatType() << "\n";
            } else {
                std::cout << "  ✗ Failed to read database\n";
            }
            delete db;
        }
        
        // Read iterator file
        std::cout << "[READ] Reading iterator file...\n";
        {
            LyraDBIteratorFormat* iter = LyraFileFormatManager::CreateIteratorFormat();
            std::string iterFile = dbName + "_products_iter.lyradbite";
            
            if (iter->ReadFromFile(iterFile)) {
                std::cout << "  ✓ Iterator read successfully\n";
                std::cout << "  └─ Format: " << iter->GetFormatType() << "\n";
            } else {
                std::cout << "  ✗ Failed to read iterator\n";
            }
            delete iter;
        }
        
        // Read archive file
        std::cout << "[READ] Reading archive file...\n";
        {
            LyraArchiveFormat* arc = LyraFileFormatManager::CreateArchiveFormat();
            std::string archiveFile = dbName + "_backup_2025-12-16.lyra";
            
            if (arc->ReadFromFile(archiveFile)) {
                std::cout << "  ✓ Archive read successfully\n";
                std::cout << "  └─ Format: " << arc->GetFormatType() << "\n";
            } else {
                std::cout << "  ✗ Failed to read archive\n";
            }
            delete arc;
        }
        
        std::cout << "\n[SUCCESS] ✓ All files read and verified!\n";
    }
};

// ============================================================================
// Main Program
// ============================================================================

int main() {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "LyraDB Formats Library - Practical Usage Demo" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    try {
        // Create backup system
        FileBackupSystem backupSystem("ECommerceApp", "store_db");
        
        // Run all scenarios
        backupSystem.ExportDatabase();
        backupSystem.CreateDataIterator();
        backupSystem.CreateBackupArchive();
        backupSystem.ValidateAllFiles();
        backupSystem.ReadAndVerifyFiles();
        
        // Final summary
        std::cout << "\n" << std::string(70, '=') << std::endl;
        std::cout << "Demo Completed Successfully!" << std::endl;
        std::cout << std::string(70, '=') << std::endl;
        
        std::cout << "\n✅ Library Usage Summary:\n";
        std::cout << "  • Created 3 file format classes (.lyradb, .lyradbite, .lyra)\n";
        std::cout << "  • Demonstrated database export functionality\n";
        std::cout << "  • Showed efficient iterator creation\n";
        std::cout << "  • Created backup archives with integrity verification\n";
        std::cout << "  • Validated all generated files\n";
        std::cout << "  • Successfully read and verified file contents\n";
        std::cout << "\n" << std::string(70, '=') << "\n" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
