# 🚀 LyraDB Quick Start (5 Minutes)

## Step 1: Choose Your Installation Method (1 min)

### 👉 **Option A: Conan (Recommended - Easiest)**
```bash
# Just 2 commands
conan install lyradb_formats/1.0.0
# Done! Ready to use
```

### Option B: Pre-built Binaries
Download from `dist/windows/release/` - Copy files to your project

### Option C: Build from Source
```bash
git clone https://github.com/Seread335/LyraDB.git
cd LyraDB
build_windows.bat
```

---

## Step 2: Create Your First Program (2 min)

### Create `main.cpp`
```cpp
#include "lyradb/lyradb_formats.h"
#include <iostream>

using namespace lyradb;

int main() {
    // Create a database file
    LyraDBFormat db;
    db.database_name = "MyFirstDB";
    db.version = 1;
    
    // Write to disk
    if (db.WriteToFile("first_db.lyradb")) {
        std::cout << "✓ Database created: first_db.lyradb" << std::endl;
    } else {
        std::cout << "✗ Failed to create database" << std::endl;
    }
    
    // Read it back
    LyraDBFormat loaded;
    if (loaded.ReadFromFile("first_db.lyradb")) {
        std::cout << "✓ Loaded: " << loaded.database_name << std::endl;
    }
    
    return 0;
}
```

---

## Step 3: Compile & Run (2 min)

### Windows (with Conan)
```bash
# Already have Conan installed?
cl /std:c++17 /I"C:\Users\<user>\.conan2\p\...\include" /EHsc main.cpp
./main.exe
```

### Windows (with Pre-built)
```bash
cl /std:c++17 /I"path/to/include" /EHsc main.cpp /link lyradb_formats.lib
./main.exe
```

### Linux/macOS
```bash
g++ -std=c++17 -I"path/to/include" main.cpp -o main
./main
```

---

## Expected Output
```
✓ Database created: first_db.lyradb
✓ Loaded: MyFirstDB
```

✅ **Congratulations! You've created your first LyraDB file!**

---

## Next Examples

### Create Iterator File
```cpp
#include "lyradb/lyradb_formats.h"
using namespace lyradb;

int main() {
    LyraDBIteratorFormat iter;
    iter.cursor_position = 0;
    iter.column_count = 3;
    iter.WriteToFile("my_iterator.lyradbite");
    return 0;
}
```

### Create Archive File
```cpp
#include "lyradb/lyradb_formats.h"
using namespace lyradb;

int main() {
    LyraArchiveFormat archive;
    archive.encryption_enabled = true;
    archive.archive_name = "backup_2024";
    archive.WriteToFile("backup.lyra");
    return 0;
}
```

---

## File Format Quick Reference

### `.lyradb` - Database Format
- Full database schema
- Table definitions
- Index information
- Metadata

**Typical use:** Save/load database snapshots

### `.lyradbite` - Iterator Format
- Cursor position
- Column information
- Page information
- Pagination support

**Typical use:** Stateful data iteration

### `.lyra` - Archive Format
- Encryption support
- Compression
- Version control
- Integrity verification

**Typical use:** Secure backups

---

## 3 More Complete Examples

### Example 1: Save Multiple Tables
```cpp
LyraDBFormat db;
db.database_name = "Shop";
db.version = 1;
// Add tables, schemas, etc.
db.WriteToFile("shop.lyradb");
```

### Example 2: Create Paginated Iterator
```cpp
LyraDBIteratorFormat iter;
iter.cursor_position = 100;      // Start at row 100
iter.page_size = 50;             // 50 rows per page
iter.total_rows = 10000;         // Total rows
iter.WriteToFile("paginated.lyradbite");
```

### Example 3: Encrypted Backup
```cpp
LyraArchiveFormat archive;
archive.encryption_enabled = true;
archive.compression_type = "zstd";  // Using ZSTD compression
archive.WriteToFile("encrypted_backup.lyra");
```

---

## Troubleshooting Quick Fixes

| Problem | Solution |
|---------|----------|
| "lyradb_formats.h not found" | Check include path in compiler flags |
| Compilation error with C++17 | Add `/std:c++17` to compiler |
| ".lib not found" | Add library path to linker |
| File write fails | Check disk space and permissions |

---

## What's Next?

1. ✅ Read [Installation Guide](INSTALLATION.md) for detailed setup
2. ✅ Check [examples/](examples/) for more examples
3. ✅ See [docs/](docs/) for full API reference
4. ✅ Try [examples/conan_usage_example/](examples/conan_usage_example/) for advanced usage

---

## Key Functions

```cpp
// Write file
bool WriteToFile(const std::string& filename);

// Read file  
bool ReadFromFile(const std::string& filename);

// Get file size
uint64_t GetFileSize(const std::string& filename);

// Verify integrity
bool VerifyIntegrity();

// Create format manager
LyraFileFormatManager manager;
auto format = manager.CreateFormat("Database");
```

---

**🎉 You're ready! Start building with LyraDB!**

**Questions?** Check [docs/](docs/) or [issues](https://github.com/Seread335/LyraDB/issues)

---

*Last Updated: 2024-Q1*
*LyraDB v1.0.0*
