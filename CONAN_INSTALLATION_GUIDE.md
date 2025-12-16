# LyraDB Formats - Conan Installation Guide

## Quick Start (3 Steps)

### 1. Install LyraDB Formats via Conan

```bash
conan install lyradb_formats/1.0.0
```

### 2. Create Your Project with conanfile.txt

Create a new project directory and add `conanfile.txt`:

```ini
[requires]
lyradb_formats/1.0.0

[generators]
CMakeDeps
CMakeToolchain

[layout]
cmake_layout
```

### 3. Use in Your CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.15)
project(my_lyradb_app)

set(CMAKE_CXX_STANDARD 17)

find_package(lyradb_formats REQUIRED CONFIG)

add_executable(my_app main.cpp)

target_link_libraries(my_app PRIVATE lyradb_formats::lyradb_formats)

# Include the source files for compilation
target_sources(my_app PRIVATE 
    ${CONAN_LYRADB_FORMATS_ROOT}/src/lyradb_formats.cpp
)
```

## Package Information

- **Package Name:** `lyradb_formats`
- **Version:** `1.0.0`
- **Type:** C++17 Header + Source Library
- **Platforms:** Windows (MSVC), Linux (GCC), macOS (Clang)

## Features

The Conan package includes:

### 3 File Formats
1. **`.lyradb`** - Database format
   - Database metadata and structure
   - Table definitions and indexes
   - Compression settings

2. **`.lyradbite`** - Iterator format
   - Cursor/iterator configuration
   - Column definitions
   - Performance statistics

3. **`.lyra`** - Archive format
   - Backup and archival
   - Encryption metadata
   - Data integrity verification

### API Classes

```cpp
#include "lyradb/lyradb_formats.h"

// Create database snapshot
LyraDB::LyraDBFormat db_format;
db_format.version = 1;
db_format.database_name = "MyDatabase";
db_format.WriteToFile("snapshot.lyradb");

// Create iterator
LyraDB::LyraDBIteratorFormat iterator;
iterator.column_count = 4;
iterator.WriteToFile("iterator.lyradbite");

// Create archive
LyraDB::LyraArchiveFormat archive;
archive.encryption_enabled = true;
archive.WriteToFile("backup.lyra");

// Detect and read files
auto format = LyraDB::LyraFileFormatManager::DetectAndRead("file.lyradb");
```

## Installation Methods

### Method 1: From Local Cache (After conan create)

```bash
cd <lyradb-project-dir>
conan create .
```

Then use in any project:

```bash
conan install lyradb_formats/1.0.0
```

### Method 2: Remote Repository (Future)

When published to Conan Center:

```bash
conan install lyradb_formats/1.0.0 -r conancenter
```

### Method 3: From GitHub

```bash
git clone https://github.com/Seread335/LyraDB.git
cd LyraDB
conan create .
```

## Complete Example Project

See `examples/conan_usage_example/` for a complete working example:

```bash
cd examples/conan_usage_example
conan install . 
cmake --preset conan-default
cmake --build --preset conan-release
./build/Release/my_app
```

## Troubleshooting

### "Package not found"
```bash
# Make sure it's created locally first
conan create <path-to-lyradb>
```

### Build failures with MSVC
Ensure C++17 is available:
```bash
conan install . -s compiler.cppstd=17
```

### CMake integration issues
Ensure CMakeDeps generator is used:
```ini
[generators]
CMakeDeps
CMakeToolchain
```

## Development

To modify and rebuild the package:

```bash
cd LyraDB
conan create . --build=missing
```

## Support

- GitHub: https://github.com/Seread335/LyraDB
- Issues: https://github.com/Seread335/LyraDB/issues

---

**Happy database development with LyraDB!** 🚀
