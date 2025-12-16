#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <ctime>

#include "lyradb/lyradb_formats.h"

int main() {
    std::cout << "=== LyraDB Formats Library - Conan Example ===\n\n";

    try {
        // Example 1: Create a database snapshot
        {
            std::cout << "[1] Creating database snapshot (.lyradb)...\n";
            
            LyraDB::LyraDBFormat db_format;
            db_format.version = 1;
            db_format.database_name = "ConanTestDB";
            db_format.created_timestamp = static_cast<uint64_t>(std::time(nullptr));
            
            // Add table metadata
            LyraDB::TableMetadata table;
            table.table_name = "users";
            table.row_count = 100;
            table.total_size = 5000;
            
            db_format.tables.push_back(table);
            
            // Write to file
            if (db_format.WriteToFile("conan_example.lyradb")) {
                std::cout << "✓ Successfully created conan_example.lyradb\n";
            } else {
                std::cout << "✗ Failed to create database file\n";
                return 1;
            }
        }

        // Example 2: Create an iterator
        {
            std::cout << "\n[2] Creating product iterator (.lyradbite)...\n";
            
            LyraDB::LyraDBIteratorFormat iterator_format;
            iterator_format.version = 1;
            iterator_format.iterator_name = "ProductCatalogIterator";
            iterator_format.column_count = 3;
            iterator_format.row_count = 50;
            
            // Add column info
            LyraDB::IteratorColumnInfo col;
            col.column_name = "product_id";
            col.data_type = "INT64";
            iterator_format.columns.push_back(col);
            
            col.column_name = "product_name";
            col.data_type = "VARCHAR";
            iterator_format.columns.push_back(col);
            
            col.column_name = "price";
            col.data_type = "DOUBLE";
            iterator_format.columns.push_back(col);
            
            if (iterator_format.WriteToFile("conan_example_products.lyradbite")) {
                std::cout << "✓ Successfully created conan_example_products.lyradbite\n";
            } else {
                std::cout << "✗ Failed to create iterator file\n";
                return 1;
            }
        }

        // Example 3: Create an archive
        {
            std::cout << "\n[3] Creating backup archive (.lyra)...\n";
            
            LyraDB::LyraArchiveFormat archive;
            archive.version = 1;
            archive.archive_name = "ConanBackup";
            archive.created_timestamp = static_cast<uint64_t>(std::time(nullptr));
            archive.encryption_enabled = true;
            archive.encryption_algorithm = "AES-256-GCM";
            
            // Add file entries
            LyraDB::ArchiveFileEntry entry;
            entry.file_path = "data/users.dat";
            entry.file_size = 10000;
            entry.compression_method = "ZSTD";
            entry.crc64_checksum = 0x0123456789ABCDEF;
            
            archive.file_entries.push_back(entry);
            
            entry.file_path = "data/products.dat";
            entry.file_size = 5000;
            archive.file_entries.push_back(entry);
            
            if (archive.WriteToFile("conan_example_backup.lyra")) {
                std::cout << "✓ Successfully created conan_example_backup.lyra\n";
            } else {
                std::cout << "✗ Failed to create archive file\n";
                return 1;
            }
        }

        // Example 4: Validate files
        {
            std::cout << "\n[4] Validating created files...\n";
            
            auto db_format = LyraDB::LyraFileFormatManager::DetectAndRead("conan_example.lyradb");
            if (db_format) {
                std::cout << "✓ Database file is valid (v" << static_cast<int>(db_format->version) << ")\n";
            }
            
            auto iter_format = LyraDB::LyraFileFormatManager::DetectAndRead("conan_example_products.lyradbite");
            if (iter_format) {
                std::cout << "✓ Iterator file is valid\n";
            }
            
            auto archive = LyraDB::LyraFileFormatManager::DetectAndRead("conan_example_backup.lyra");
            if (archive) {
                std::cout << "✓ Archive file is valid\n";
            }
        }

        std::cout << "\n✓ All Conan examples completed successfully!\n\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "✗ Error: " << e.what() << "\n";
        return 1;
    }
}
