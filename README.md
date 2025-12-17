# 📦 LyraDB - Production Database Engine

![LyraDB](https://img.shields.io/badge/Version-1.0.0-blue)
![C++17](https://img.shields.io/badge/C%2B%2B-17-brightgreen)
![License](https://img.shields.io/badge/License-MIT-green)
![Build](https://img.shields.io/badge/Build-Windows%2FLinux%2FmacOS-brightgreen)

A high-performance, feature-rich relational database engine written in modern C++17 with production-grade stability and optimization.

## 🚀 Quick Start (Choose Your Path)

### ⭐ Option 1: Conan Package (Easiest - Recommended)

```bash
# Just 2 commands!
conan install lyradb_formats/1.0.0
# Then use in your project - no compilation needed!
```

**Perfect for:** Quick integration, cross-platform projects, CI/CD pipelines

### Option 2: From Source (Full Control)

```bash
git clone https://github.com/Seread335/LyraDB.git
cd LyraDB

# Windows
build_windows.bat

# Linux
./build_linux.sh

# macOS
./build_macos.sh
```

**Perfect for:** Building custom, debugging, contributing to LyraDB

### Option 3: Pre-built Binaries

Download from `dist/` folder - Windows Release/Debug builds ready to use.

**Perfect for:** Quick testing without compilation

👉 **[Detailed Installation Guide →](INSTALLATION.md)**

## ✨ Features

### Core Database Engine
- ✅ **Full SQL Support** - SELECT, INSERT, UPDATE, DELETE, JOIN, GROUP BY, ORDER BY
- ✅ **Advanced Query Optimization** - Multiple optimizer phases for optimal execution
- ✅ **Indexing** - B-Tree and Hash indexes with smart index selection
- ✅ **Buffer Management** - LRU2 replacement policy for memory efficiency

### Data Storage & Compression
- ✅ **5 Compression Methods** - Bitpacking, Delta, Dictionary, RLE, ZSTD
- ✅ **Automatic Compression Selection** - Intelligent compression chooser
- ✅ **Column-Oriented Storage** - Efficient data layout for analytics

### 3 File Formats (via lyradb_formats library)
- 📄 **`.lyradb`** - Database snapshots with full metadata
- 🔄 **`.lyradbite`** - Iterator/cursor for sequential access
- 📦 **`.lyra`** - Encrypted backup archives with integrity verification

### APIs
- ✅ **C++ API** - Modern C++17 interface
- ✅ **C API** - Full C bindings for language compatibility
- ✅ **REST API** - HTTP server for remote access

## 📚 Documentation

### Installation & Setup
| Document | Purpose |
|----------|---------|
| **[Installation Guide](INSTALLATION.md)** | 📖 **START HERE** - All 3 installation options |
| **[Conan Guide](CONAN_INSTALLATION_GUIDE.md)** | Package installation via Conan |
| [Getting Started](docs/01_GETTING_STARTED.md) | First steps and basic examples |

### API References
| Document | Purpose |
|----------|---------|
| [C++ API Reference](docs/04_CPP_API_REFERENCE.md) | C++ API documentation |
| [C API Reference](docs/03_C_API_REFERENCE.md) | C API documentation |
| [SQL Reference](docs/02_SQL_REFERENCE.md) | SQL syntax and examples |

### Advanced Topics
| Document | Purpose |
|----------|---------|
| [Data Types Reference](docs/05_DATA_TYPES_REFERENCE.md) | Supported data types |
| [Integration Guide](docs/08_INTEGRATION_GUIDE.md) | System integration |
| [Troubleshooting](docs/06_TROUBLESHOOTING.md) | Common issues & solutions |

### Project Info
| Document | Purpose |
|----------|---------|
| [Test Reports](DATABASE_COMPREHENSIVE_TEST_REPORT.md) | 95%+ test pass rate results |
| [Issues Fixed](ISSUES_FIXED.md) | Completed bug fixes & improvements |
| [Usage & Distribution](USAGE_AND_DISTRIBUTION.md) | Project distribution details |

## 📦 Library Usage

### Using lyradb_formats via Conan

```cpp
#include "lyradb/lyradb_formats.h"

// Create database snapshot
LyraDB::LyraDBFormat db;
db.database_name = "MyDB";
db.WriteToFile("snapshot.lyradb");

// Create iterator
LyraDB::LyraDBIteratorFormat iterator;
iterator.WriteToFile("data.lyradbite");

// Create archive
LyraDB::LyraArchiveFormat archive;
archive.encryption_enabled = true;
archive.WriteToFile("backup.lyra");
```

See [examples/conan_usage_example](examples/conan_usage_example/) for complete working code.

## 🔧 Building

### Prerequisites
- C++17 compiler (MSVC 19.4+, GCC 9+, Clang 10+)
- CMake 3.20+
- Optional: Conan 2.0+

### Windows
```bash
build_windows.bat
```

### Linux
```bash
./build_linux.sh
```

### macOS
```bash
./build_macos.sh
```

## 📊 Benchmarks

LyraDB includes comprehensive benchmarks:
- **Query Performance** - Optimized query execution
- **Compression Ratio** - Up to 10x data compression
- **Index Performance** - O(log n) lookups with B-Tree
- **Buffer Management** - Efficient memory utilization

Run benchmarks:
```bash
cd build
./Release/phase44_benchmark.exe
./Release/bench_queries.exe
```

## 🧪 Testing

Comprehensive test suite included:

```bash
cd build
# Run individual tests
./Release/test_storage.exe
./Release/test_query.exe
./Release/test_compression.exe

# Or run all tests
cmake --build . --target test --config Release
```

## 📋 Test Results

✅ **95%+ Test Pass Rate**
- Storage & Serialization: PASS
- Query Execution: PASS  
- Compression: PASS
- Indexing: PASS
- Buffer Management: PASS
- Integration: PASS

See [DATABASE_COMPREHENSIVE_TEST_REPORT.md](DATABASE_COMPREHENSIVE_TEST_REPORT.md) for full test results.

## 🌐 API Examples

### C++ Example
```cpp
#include "lyradb.h"

int main() {
    // Create database
    LyraDB::Database db("mydb");
    
    // Create table
    LyraDB::Schema schema("users");
    schema.AddColumn("id", LyraDB::INT64);
    schema.AddColumn("name", LyraDB::VARCHAR);
    db.CreateTable(schema);
    
    // Execute query
    auto result = db.ExecuteQuery("SELECT * FROM users WHERE id > 10");
    
    return 0;
}
```

### C Example
```c
#include "lyradb_c.h"

int main() {
    lyradb_database_t db = lyradb_create_database("mydb");
    lyradb_query_result_t result = lyradb_execute_query(db, "SELECT * FROM users");
    lyradb_close_database(db);
    return 0;
}
```

## 📁 Project Structure

```
LyraDB/
├── include/           # Public headers
│   ├── lyradb.h      # Main C++ API
│   ├── lyradb_c.h    # C API
│   └── lyradb/       # Library headers (lyradb_formats.h, etc.)
├── src/              # Implementation
│   ├── core/        # Database core
│   ├── query/       # Query processing
│   ├── storage/     # Storage & compression
│   ├── indexes/     # Index implementations
│   ├── buffer/      # Buffer management
│   ├── execution/   # Execution engine
│   ├── server/      # REST API server
│   └── bindings/    # C API bindings
├── examples/        # Working examples
│   ├── conan_usage_example/    # Conan package example
│   ├── test_formats.cpp         # File format examples
│   └── usage_demo.cpp           # Production demo
├── tests/           # Test suite
├── benchmarks/      # Performance benchmarks
├── docs/            # Documentation (9 guides)
├── test_web_app/    # E-commerce web app demo
├── dist/            # Pre-built distributions
│   ├── windows/     # Windows builds (Release/Debug)
│   ├── documentation/  # Distribution docs
│   ├── libraries/    # Library distributions
│   └── examples/     # Distribution examples
└── build/           # Build output (generated)
```

## 🎯 Key Achievements

- ✅ Production-grade database engine
- ✅ Comprehensive query optimization (7+ optimizer phases)
- ✅ Multiple data compression algorithms
- ✅ Advanced indexing strategies
- ✅ Full ACID compliance support
- ✅ REST API server
- ✅ C/C++ bindings
- ✅ Conan package support
- ✅ 197 files, 70K+ lines of code
- ✅ Extensive test coverage

## 🤝 Contributing

Contributions welcome! Please check [ISSUES_FIXED.md](ISSUES_FIXED.md) for known issues and completed work.

## 📄 License

MIT License - see [LICENSE](LICENSE) file

## 👨‍💻 Author

LyraDB Team

## 🔗 Links

- **GitHub:** https://github.com/Seread335/LyraDB
- **Conan Package:** `lyradb_formats/1.0.0`
- **Documentation:** See `/docs` folder

---

**Built with ❤️ for performance, stability, and ease of use.**
