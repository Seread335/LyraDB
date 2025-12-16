# LyraDB Integration Guide

This guide explains how to integrate LyraDB into your project, similar to how you would use SQLite.

## Quick Integration (3 Steps)

### Step 1: Copy LyraDB to Your Project

For a project structure like:
```
MyProject/
  src/
    main.cpp
  CMakeLists.txt
  .gitignore
```

Copy LyraDB distribution:
```
MyProject/
  src/
    main.cpp
  third_party/
    lyradb/              <- Copy here
      include/
      lib/
  CMakeLists.txt
```

### Step 2: Update Your CMakeLists.txt

For C++ projects:
```cmake
# Set C++11 minimum
set(CMAKE_CXX_STANDARD 11)

# Add LyraDB
add_subdirectory(third_party/lyradb)
include_directories(third_party/lyradb/include)

# Your executable
add_executable(myapp src/main.cpp)
target_link_libraries(myapp lyradb_core)
```

For C projects:
```cmake
set(CMAKE_C_STANDARD 99)

add_subdirectory(third_party/lyradb)
include_directories(third_party/lyradb/include)

add_executable(myapp src/main.c)
target_link_libraries(myapp lyradb_c)
```

### Step 3: Include and Use

C++ usage:
```cpp
#include "lyradb/database.h"
#include "lyradb/sql_parser.h"

int main() {
    lyradb::Database db;
    auto parser = lyradb::SQLParser::create();
    auto stmt = parser->parse("CREATE TABLE users (id INT32, name STRING)");
    db.execute(stmt);
    return 0;
}
```

C usage:
```c
#include "lyradb_c.h"

int main() {
    lyradb_handle_t db = lyradb_create_database();
    lyradb_execute(db, "CREATE TABLE users (id INT32, name STRING)");
    lyradb_destroy_database(db);
    return 0;
}
```

## Cross-Platform Setup

### Using Prebuilt Binaries

If prebuilt binaries are available for your platform:

Linux:
```bash
mkdir -p third_party/lyradb/lib
cp dist/linux/lib/liblyradb.so third_party/lyradb/lib/
cp dist/linux/lib/liblyradb.a third_party/lyradb/lib/
cp -r dist/linux/include/* third_party/lyradb/include/
```

macOS:
```bash
mkdir -p third_party/lyradb/lib
cp dist/macos/lib/liblyradb.dylib third_party/lyradb/lib/
cp dist/macos/lib/liblyradb.a third_party/lyradb/lib/
cp -r dist/macos/include/* third_party/lyradb/include/
```

Windows:
```batch
mkdir third_party\lyradb\lib
copy dist\windows\lib\lyradb.dll third_party\lyradb\lib\
copy dist\windows\lib\lyradb.lib third_party\lyradb\lib\
xcopy /E dist\windows\include third_party\lyradb\include
```

### Building from Source

For your specific platform, build LyraDB:

```bash
cd dist-source
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
sudo make install
```

Then in your CMakeLists.txt:
```cmake
find_package(LyraDB REQUIRED)
target_link_libraries(myapp LyraDB::lyradb_core)
```

## CMake Integration Examples

### Example 1: Header-Only Style (Recommended)

```cmake
# In your CMakeLists.txt
set(LYRADB_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/lyradb/include")
include_directories(${LYRADB_INCLUDE_DIR})

# Link with pre-built library
if(WIN32)
    link_directories("${CMAKE_SOURCE_DIR}/third_party/lyradb/lib/windows")
    target_link_libraries(myapp lyradb.lib)
elseif(APPLE)
    link_directories("${CMAKE_SOURCE_DIR}/third_party/lyradb/lib/macos")
    target_link_libraries(myapp lyradb)
else()
    link_directories("${CMAKE_SOURCE_DIR}/third_party/lyradb/lib/linux")
    target_link_libraries(myapp lyradb)
endif()
```

### Example 2: Subdirectory Approach

```cmake
# In third_party/lyradb/CMakeLists.txt (if building from source)
add_library(lyradb_static STATIC
    src/core/database.cpp
    src/query/sql_parser.cpp
    # ... other source files
)

target_include_directories(lyradb_static PUBLIC include)

# In main CMakeLists.txt
add_subdirectory(third_party/lyradb)
target_link_libraries(myapp lyradb_static)
```

## Compiler Flags

For optimal performance, use:

GCC/Clang:
```cmake
set(CMAKE_CXX_FLAGS "-O3 -march=native")
```

MSVC:
```cmake
set(CMAKE_CXX_FLAGS "/O2 /arch:AVX2")
```

## Configuration Options

LyraDB supports compile-time options:

```cmake
option(LYRADB_ENABLE_SIMD "Enable SIMD optimizations" ON)
option(LYRADB_ENABLE_COMPRESSION "Enable compression engines" ON)
option(BUILD_STATIC "Build static library instead of shared" ON)
```

## Platform-Specific Notes

### Windows

- Visual Studio 2015 or later required
- Supports both 32-bit and 64-bit builds
- DLL is dynamically linked; ensure it's available at runtime
- Static library recommended for distribution

Linking with DLL:
```cmake
target_link_libraries(myapp lyradb.lib)
```

Linking with static:
```cmake
target_link_libraries(myapp lyradb_static.lib)
```

### Linux

- GCC 4.8+ or Clang 3.3+ required
- Use shared library (.so) for smaller binary size
- Use static library (.a) for portability

```cmake
# Shared library
target_link_libraries(myapp liblyradb.so)

# Static library
target_link_libraries(myapp liblyradb.a)
```

### macOS

- Xcode 7.0 or later required
- Use dylib for development, static for distribution
- Ensure correct architecture (Intel x86_64 or Apple Silicon arm64)

```cmake
# Check architecture
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
    # Apple Silicon (M1/M2)
    message(STATUS "Building for Apple Silicon")
else()
    # Intel
    message(STATUS "Building for Intel Mac")
endif()
```

## Verifying Installation

Create a simple test program:

```cpp
#include <iostream>
#include "lyradb/database.h"
#include "lyradb/sql_parser.h"

int main() {
    try {
        lyradb::Database db;
        auto parser = lyradb::SQLParser::create();
        
        auto stmt = parser->parse("CREATE TABLE test (id INT32)");
        if (!stmt) {
            std::cerr << parser->error() << std::endl;
            return 1;
        }
        
        auto result = db.execute(stmt);
        std::cout << "LyraDB initialized successfully!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
```

Build and run:
```bash
cmake ..
make
./test_lyradb
# Output: LyraDB initialized successfully!
```

## Troubleshooting Integration

### Linker Error: Cannot find library

Solution:
1. Verify library file exists in lib/ directory
2. Check library name matches in CMakeLists.txt
3. Use absolute path: `link_directories("/absolute/path/to/lib")`

### Header Not Found

Solution:
1. Verify headers exist in include/ directory
2. Check include path in CMakeLists.txt
3. Use full path: `include_directories("/absolute/path/to/include")`

### Symbol Not Found (macOS)

Solution:
1. Verify library was built for your architecture
2. Check dylib dependencies: `otool -L liblyradb.dylib`
3. Set DYLD_LIBRARY_PATH: `export DYLD_LIBRARY_PATH=/path/to/lib`

### Runtime Library Not Found (Windows)

Solution:
1. Place .dll in same directory as executable
2. Or add dll directory to PATH
3. Or use static library instead

## Best Practices

1. Use static library for distribution/deployment
2. Use shared library during development for faster iteration
3. Always check build output for warnings
4. Test on target platforms before distribution
5. Keep LyraDB in version control (in third_party/)
6. Document LyraDB version in your project
7. Use consistent C++ standard (C++11 minimum)

## Version Management

Track LyraDB version in your project:

```cmake
# In your CMakeLists.txt
set(LYRADB_VERSION "1.2.0")
message(STATUS "Using LyraDB v${LYRADB_VERSION}")

# Add to build info
add_compile_definitions(LYRADB_VERSION="${LYRADB_VERSION}")
```

## Performance Tips

1. Build with Release mode: `-DCMAKE_BUILD_TYPE=Release`
2. Enable optimizations: `-O3` (GCC/Clang), `/O2` (MSVC)
3. Enable SIMD: `-march=native`, `/arch:AVX2`
4. Use static linking for maximum performance
5. Profile your application: `perf record`, `Instruments`, etc.

## Next Steps

1. Copy LyraDB distribution to your project
2. Update CMakeLists.txt to link LyraDB
3. Include headers in your source
4. Write test program to verify integration
5. Start using LyraDB in your application
6. Consult LyraDB documentation for advanced features
