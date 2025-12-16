/**
 * LyraDB .db File Usage Example
 * 
 * Demonstrates how to create, save, and load .db files
 * Just like SQLite!
 */

#include <iostream>
#include <fstream>
#include <filesystem>

// This is a simplified example showing the concept
// In real usage, you would use the DatabaseFile class

int main() {
    std::cout << "═══════════════════════════════════════════════════════════\n";
    std::cout << "  LyraDB .db File Format - Quick Start\n";
    std::cout << "═══════════════════════════════════════════════════════════\n\n";
    
    std::cout << "LyraDB now supports .db files - just like SQLite!\n\n";
    
    // ===================================================================
    // Example 1: Create and Save
    // ===================================================================
    std::cout << "📁 EXAMPLE 1: Create and Save Database\n";
    std::cout << "───────────────────────────────────────────────────────────\n\n";
    
    std::cout << "Code:\n";
    std::cout << "  lyradb::DatabaseFile db(\"myapp.db\");\n";
    std::cout << "  db.execute(\"CREATE TABLE users (id INT32, name STRING)\");\n";
    std::cout << "  db.execute(\"INSERT INTO users VALUES (1, 'Alice')\");\n";
    std::cout << "  db.save();\n\n";
    
    std::cout << "Result:\n";
    std::cout << "  ✓ File created: myapp.db\n";
    std::cout << "  ✓ Size: ~5-10 KB (depending on data)\n";
    std::cout << "  ✓ Can be copied, shared, backed up\n\n";
    
    // ===================================================================
    // Example 2: Load Existing File
    // ===================================================================
    std::cout << "📁 EXAMPLE 2: Load Existing .db File\n";
    std::cout << "───────────────────────────────────────────────────────────\n\n";
    
    std::cout << "Code:\n";
    std::cout << "  auto db = lyradb::DatabaseFile::open(\"myapp.db\");\n";
    std::cout << "  auto result = db.execute(\"SELECT * FROM users\");\n\n";
    
    std::cout << "Result:\n";
    std::cout << "  ✓ Database loaded from file\n";
    std::cout << "  ✓ All data available immediately\n";
    std::cout << "  ✓ Ready to query or modify\n\n";
    
    // ===================================================================
    // Example 3: Multiple Files
    // ===================================================================
    std::cout << "📁 EXAMPLE 3: Multiple .db Files\n";
    std::cout << "───────────────────────────────────────────────────────────\n\n";
    
    std::cout << "You can have multiple .db files for different purposes:\n\n";
    
    std::cout << "  users.db\n";
    std::cout << "    ├─ users table\n";
    std::cout << "    └─ roles table\n\n";
    
    std::cout << "  products.db\n";
    std::cout << "    ├─ products table\n";
    std::cout << "    ├─ categories table\n";
    std::cout << "    └─ inventory table\n\n";
    
    std::cout << "  logs.db\n";
    std::cout << "    ├─ audit_log table\n";
    std::cout << "    └─ error_log table\n\n";
    
    std::cout << "Code:\n";
    std::cout << "  lyradb::DatabaseFile users_db(\"users.db\");\n";
    std::cout << "  lyradb::DatabaseFile products_db(\"products.db\");\n";
    std::cout << "  lyradb::DatabaseFile logs_db(\"logs.db\");\n\n";
    
    // ===================================================================
    // Example 4: File Format
    // ===================================================================
    std::cout << "📋 EXAMPLE 4: .db File Format\n";
    std::cout << "───────────────────────────────────────────────────────────\n\n";
    
    std::cout << "Binary format (like SQLite):\n\n";
    std::cout << "  ┌────────────────────────────────────┐\n";
    std::cout << "  │  Header (12 bytes)                 │\n";
    std::cout << "  │  - Magic: \"LYDB\" (4 bytes)        │\n";
    std::cout << "  │  - Version: 1 (4 bytes)           │\n";
    std::cout << "  │  - Timestamp (8 bytes)            │\n";
    std::cout << "  ├────────────────────────────────────┤\n";
    std::cout << "  │  Metadata                          │\n";
    std::cout << "  │  - Table definitions               │\n";
    std::cout << "  │  - Column schemas                  │\n";
    std::cout << "  │  - Index definitions               │\n";
    std::cout << "  ├────────────────────────────────────┤\n";
    std::cout << "  │  Data Section                      │\n";
    std::cout << "  │  - Column 1 data (compressed)     │\n";
    std::cout << "  │  - Column 2 data (compressed)     │\n";
    std::cout << "  │  - ... more columns               │\n";
    std::cout << "  └────────────────────────────────────┘\n\n";
    
    // ===================================================================
    // Example 5: Operations
    // ===================================================================
    std::cout << "🔧 EXAMPLE 5: Common Operations\n";
    std::cout << "───────────────────────────────────────────────────────────\n\n";
    
    std::cout << "Create database:\n";
    std::cout << "  lyradb::DatabaseFile db(\"app.db\");\n\n";
    
    std::cout << "Save to file:\n";
    std::cout << "  db.save();\n\n";
    
    std::cout << "Open existing file:\n";
    std::cout << "  auto db = lyradb::DatabaseFile::open(\"app.db\");\n\n";
    
    std::cout << "Execute SQL:\n";
    std::cout << "  auto result = db.execute(\"SELECT * FROM table\");\n\n";
    
    std::cout << "Save as backup:\n";
    std::cout << "  db.save_as(\"app_backup.db\");\n\n";
    
    std::cout << "Create backup:\n";
    std::cout << "  db.backup(\"app_2025-12-12.db\");\n\n";
    
    std::cout << "Compact (remove deleted space):\n";
    std::cout << "  db.compact();\n\n";
    
    std::cout << "Get file size:\n";
    std::cout << "  size_t bytes = db.get_file_size();\n\n";
    
    std::cout << "Close database:\n";
    std::cout << "  db.close();\n\n";
    
    // ===================================================================
    // Example 6: File Size Estimates
    // ===================================================================
    std::cout << "📊 EXAMPLE 6: File Size Estimates\n";
    std::cout << "───────────────────────────────────────────────────────────\n\n";
    
    std::cout << "  Database Type              │ File Size\n";
    std::cout << "  ───────────────────────────┼───────────────────\n";
    std::cout << "  Empty database             │ ~1 KB\n";
    std::cout << "  Small table (100 rows)     │ ~5-10 KB\n";
    std::cout << "  Medium table (10K rows)    │ ~50-100 KB\n";
    std::cout << "  Large table (1M rows)      │ ~5-50 MB\n";
    std::cout << "  Multiple tables            │ Depends on data\n\n";
    
    // ===================================================================
    // Example 7: Best Practices
    // ===================================================================
    std::cout << "✅ EXAMPLE 7: Best Practices\n";
    std::cout << "───────────────────────────────────────────────────────────\n\n";
    
    std::cout << "1. Save regularly:\n";
    std::cout << "   db.execute(\"INSERT INTO table VALUES (...)\");\n";
    std::cout << "   db.save();  // Don't forget!\n\n";
    
    std::cout << "2. Create backups:\n";
    std::cout << "   db.backup(\"important_2025-12-12.db\");\n\n";
    
    std::cout << "3. Handle errors:\n";
    std::cout << "   try {\n";
    std::cout << "       db.save();\n";
    std::cout << "   } catch (const std::exception& e) {\n";
    std::cout << "       std::cerr << \"Error: \" << e.what();\n";
    std::cout << "   }\n\n";
    
    std::cout << "4. Use meaningful names:\n";
    std::cout << "   lyradb::DatabaseFile users_db(\"users.db\");\n";
    std::cout << "   lyradb::DatabaseFile config_db(\"config.db\");\n\n";
    
    std::cout << "5. Compact after deletions:\n";
    std::cout << "   db.execute(\"DELETE FROM table WHERE id > 1000\");\n";
    std::cout << "   db.compact();  // Reclaim space\n\n";
    
    // ===================================================================
    // Key Features
    // ===================================================================
    std::cout << "🎯 KEY FEATURES\n";
    std::cout << "───────────────────────────────────────────────────────────\n\n";
    
    std::cout << "✓ Simple API - just like SQLite\n";
    std::cout << "✓ Portable - works on Windows/Linux/macOS\n";
    std::cout << "✓ Shareable - send .db files to others\n";
    std::cout << "✓ Self-contained - one file = complete database\n";
    std::cout << "✓ Compressed - data is automatically compressed\n";
    std::cout << "✓ Backup-friendly - easy to copy and backup\n";
    std::cout << "✓ Fast access - load and query in milliseconds\n\n";
    
    // ===================================================================
    // Summary
    // ===================================================================
    std::cout << "📝 SUMMARY\n";
    std::cout << "───────────────────────────────────────────────────────────\n\n";
    
    std::cout << "Using LyraDB .db files is straightforward:\n\n";
    
    std::cout << "1. Create database:        lyradb::DatabaseFile db(\"app.db\");\n";
    std::cout << "2. Execute SQL:            db.execute(\"CREATE TABLE ...\");\n";
    std::cout << "3. Save to file:           db.save();\n";
    std::cout << "4. Load from file:         db = DatabaseFile::open(\"app.db\");\n";
    std::cout << "5. Query data:             db.execute(\"SELECT ...\");\n";
    std::cout << "6. Backup:                 db.backup(\"app_backup.db\");\n\n";
    
    std::cout << "It's that simple! Now you can easily distribute and share\n";
    std::cout << "your LyraDB databases just like SQLite!\n\n";
    
    std::cout << "═══════════════════════════════════════════════════════════\n";
    std::cout << "For more details, see: docs/09_DATABASE_FILE_FORMAT.md\n";
    std::cout << "═══════════════════════════════════════════════════════════\n";
    
    return 0;
}
