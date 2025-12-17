/*
 * LyraDB Format Library - Header File
 * Defines interfaces for working with .lyradb, .lyradbite, and .lyra file formats
 * 
 * Author: LyraDB Team
 * Date: December 16, 2025
 * Version: 1.0
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <cstdint>
#include <fstream>
#include <sstream>

namespace lyradb {

// ============================================================================
// File Format Constants
// ============================================================================

const std::string LYRADB_MAGIC = "LYRADB";
const std::string LYRADBITE_MAGIC = "LYRADBITE";
const std::string LYRA_MAGIC = "LYRA";

const std::string LYRADB_EXTENSION = ".lyradb";
const std::string LYRADBITE_EXTENSION = ".lyradbite";
const std::string LYRA_EXTENSION = ".lyra";

// ============================================================================
// Base File Format Class
// ============================================================================

class LyraDBFileFormat {
public:
    virtual ~LyraDBFileFormat() = default;
    
    virtual bool WriteToFile(const std::string& filename) = 0;
    virtual bool ReadFromFile(const std::string& filename) = 0;
    virtual std::string ToString() const = 0;
    virtual std::string GetFormatType() const = 0;
};

// ============================================================================
// .lyradb - Database Format
// ============================================================================

class LyraDBFormat : public LyraDBFileFormat {
public:
    struct TableInfo {
        std::string name;
        uint32_t rowCount;
        uint32_t sizeKB;
    };
    
    struct IndexInfo {
        std::string name;
        std::string type;      // "B-Tree" or "Hash"
        std::string tableName;
        std::string columnName;
    };
    
    struct CompressionStats {
        double rleRatio;
        double deltaRatio;
        double dictionaryRatio;
        double bitPackingRatio;
        double zstdRatio;
        std::string selected;
    };
    
    // Constructor
    LyraDBFormat();
    
    // Metadata setters
    void SetDatabaseName(const std::string& name);
    void SetCreationTime(const std::string& time);
    void SetLastModified(const std::string& time);
    void SetTotalTables(uint32_t count);
    void SetTotalRows(uint32_t count);
    void SetDataSize(const std::string& size);
    void SetCompressedSize(const std::string& size);
    void SetCompressionRatio(double ratio);
    
    // Table operations
    void AddTable(const TableInfo& table);
    void AddIndex(const IndexInfo& index);
    void SetCompressionStats(const CompressionStats& stats);
    
    // Recovery and security
    void SetRecoveryLogEnabled(bool enabled);
    void SetSchemaHash(const std::string& hash);
    void SetEncryptionStatus(const std::string& status);
    
    // File operations
    bool WriteToFile(const std::string& filename) override;
    bool ReadFromFile(const std::string& filename) override;
    
    // Utility
    std::string ToString() const override;
    std::string GetFormatType() const override { return "DATABASE"; }
    
    // Getters
    std::string GetDatabaseName() const { return databaseName; }
    uint32_t GetTotalTables() const { return totalTables; }
    uint32_t GetTotalRows() const { return totalRows; }
    const std::vector<TableInfo>& GetTables() const { return tables; }
    const std::vector<IndexInfo>& GetIndexes() const { return indexes; }
    
private:
    // Metadata
    std::string databaseName;
    std::string creationTime;
    std::string lastModified;
    uint32_t totalTables;
    uint32_t totalRows;
    std::string dataSize;
    std::string compressedSize;
    double compressionRatio;
    
    // Collections
    std::vector<TableInfo> tables;
    std::vector<IndexInfo> indexes;
    
    // Compression and security
    CompressionStats compressionStats;
    bool recoveryLogEnabled;
    std::string schemaHash;
    std::string encryptionStatus;
    std::string lastCheckpoint;
};

// ============================================================================
// .lyradbite - Iterator Format
// ============================================================================

class LyraDBIteratorFormat : public LyraDBFileFormat {
public:
    struct ColumnMapping {
        std::string name;
        std::string type;
        std::string size;
    };
    
    struct IterationConfig {
        uint32_t bufferSize;
        bool cachingEnabled;
        uint32_t prefetchSize;
        uint32_t batchSize;
        std::string compression;
    };
    
    struct CursorInfo {
        uint64_t startOffset;
        uint64_t endOffset;
        uint64_t currentPosition;
        std::string direction;   // "FORWARD" or "BACKWARD"
        std::string status;      // "INITIALIZED", "ACTIVE", "EOF"
    };
    
    struct PerformanceStats {
        uint64_t totalPagesRead;
        uint64_t bufferHits;
        uint64_t bufferMisses;
        std::string averageRowSize;
        std::string estimatedIterationTime;
        std::string throughputExpected;
    };
    
    // Constructor
    LyraDBIteratorFormat();
    
    // Metadata setters
    void SetIteratorName(const std::string& name);
    void SetSourceDatabase(const std::string& dbname);
    void SetSourceTable(const std::string& tablename);
    void SetRowCount(uint32_t count);
    void SetPageSize(uint32_t size);
    
    // Configuration
    void SetIterationConfig(const IterationConfig& config);
    void SetCursorInfo(const CursorInfo& info);
    void SetPerformanceStats(const PerformanceStats& stats);
    
    // Column mapping
    void AddColumn(const ColumnMapping& column);
    
    // Optimization
    void SetIndexUsage(const std::string& primaryIndex);
    void EnablePrefetch(bool enabled);
    void EnableParallelization(uint32_t threads);
    
    // File operations
    bool WriteToFile(const std::string& filename) override;
    bool ReadFromFile(const std::string& filename) override;
    
    // Utility
    std::string ToString() const override;
    std::string GetFormatType() const override { return "ITERATOR"; }
    
    // Getters
    const std::vector<ColumnMapping>& GetColumns() const { return columns; }
    const CursorInfo& GetCursorInfo() const { return cursorInfo; }
    const PerformanceStats& GetPerformanceStats() const { return perfStats; }
    
private:
    // Metadata
    std::string iteratorName;
    std::string createdDate;
    std::string sourceDatabase;
    std::string sourceTable;
    uint32_t rowCount;
    uint32_t pageSize;
    
    // Configuration
    IterationConfig config;
    CursorInfo cursorInfo;
    PerformanceStats perfStats;
    
    // Collections
    std::vector<ColumnMapping> columns;
    
    // Optimization
    std::string primaryIndex;
    bool prefetchEnabled;
    uint32_t parallelizationThreads;
};

// ============================================================================
// .lyra - Archive Format
// ============================================================================

class LyraArchiveFormat : public LyraDBFileFormat {
public:
    struct ArchiveEntry {
        std::string filename;
        std::string description;
        std::string size;
    };
    
    struct BackupSchedule {
        std::string fullBackupInterval;
        std::string incrementalBackupInterval;
        std::string lastFullBackup;
        std::string nextFullBackup;
        uint32_t retentionDays;
    };
    
    struct EncryptionInfo {
        std::string encryptionMethod;    // "AES-256-GCM"
        std::string status;              // "AVAILABLE", "DISABLED"
        std::string keyDerivation;       // "PBKDF2"
        uint32_t iterationCount;
    };
    
    struct IntegrityVerification {
        std::string checksumAlgorithm;   // "CRC64"
        std::string databaseChecksum;
        uint32_t totalEntryCount;
        std::string integrityStatus;     // "VERIFIED", "FAILED"
    };
    
    // Constructor
    LyraArchiveFormat();
    
    // Archive metadata
    void SetArchiveName(const std::string& name);
    void SetCreationDate(const std::string& date);
    void SetBackupType(const std::string& type);  // "FULL", "INCREMENTAL"
    void SetSourceSystem(const std::string& system);
    void SetCompressionLevel(int level);
    
    // Database info
    void SetDatabaseName(const std::string& name);
    void SetDatabaseVersion(const std::string& version);
    void SetTablesIncluded(uint32_t count);
    void SetTotalRowsArchived(uint32_t count);
    void SetUncompressedSize(const std::string& size);
    void SetCompressedSize(const std::string& size);
    
    // Archive contents
    void AddEntry(const ArchiveEntry& entry);
    
    // Verification and security
    void SetIntegrityVerification(const IntegrityVerification& verification);
    void SetBackupSchedule(const BackupSchedule& schedule);
    void SetEncryptionInfo(const EncryptionInfo& encryption);
    
    // Versioning
    void SetSchemaVersion(const std::string& version);
    void SetDataFormatVersion(const std::string& version);
    void SetArchiveFormatVersion(const std::string& version);
    
    // File operations
    bool WriteToFile(const std::string& filename) override;
    bool ReadFromFile(const std::string& filename) override;
    
    // Utility
    std::string ToString() const override;
    std::string GetFormatType() const override { return "ARCHIVE"; }
    
    // Getters
    const std::vector<ArchiveEntry>& GetEntries() const { return entries; }
    const IntegrityVerification& GetIntegrityInfo() const { return integrityInfo; }
    uint32_t GetTotalEntryCount() const { return entries.size(); }
    
private:
    // Metadata
    std::string archiveName;
    std::string creationDate;
    std::string backupType;
    std::string sourceSystem;
    int compressionLevel;
    
    // Database info
    std::string databaseName;
    std::string databaseVersion;
    uint32_t tablesIncluded;
    uint32_t totalRowsArchived;
    std::string uncompressedSize;
    std::string compressedSize;
    
    // Collections
    std::vector<ArchiveEntry> entries;
    
    // Security and verification
    IntegrityVerification integrityInfo;
    BackupSchedule backupSchedule;
    EncryptionInfo encryptionInfo;
    
    // Versioning
    std::string schemaVersion;
    std::string dataFormatVersion;
    std::string archiveFormatVersion;
    std::string archiveID;
};

// ============================================================================
// File Format Manager - Factory and Utilities
// ============================================================================

class LyraFileFormatManager {
public:
    // Factory methods
    static LyraDBFormat* CreateDatabaseFormat();
    static LyraDBIteratorFormat* CreateIteratorFormat();
    static LyraArchiveFormat* CreateArchiveFormat();
    
    // File detection
    static std::string DetectFormatType(const std::string& filename);
    
    // Validation
    static bool IsValidLyraDBFile(const std::string& filename);
    static bool IsValidIteratorFile(const std::string& filename);
    static bool IsValidArchiveFile(const std::string& filename);
    
    // Utility functions
    static std::string GetFileExtension(const std::string& filename);
    static bool FileExists(const std::string& filename);
    static uint64_t GetFileSize(const std::string& filename);
    
private:
    static const std::map<std::string, std::string> MAGIC_SIGNATURES;
};

// ============================================================================
// Helper Functions
// ============================================================================

// String utilities
std::string TrimString(const std::string& str);
std::vector<std::string> SplitString(const std::string& str, char delimiter);
std::string JoinStrings(const std::vector<std::string>& parts, const std::string& delimiter);

// Checksum utilities
std::string CalculateCRC64(const std::string& data);
bool VerifyCRC64(const std::string& data, const std::string& checksum);

// Time utilities
std::string GetCurrentTimestamp();
std::string GetFormattedDate();

} // namespace lyradb
