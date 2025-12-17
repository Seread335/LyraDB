# Binary Download and Installation Guide

This guide provides instructions for downloading, extracting, and integrating pre-compiled LyraDB binaries.

## Step 1: Choose Your Platform

Determine your system architecture and operating system:

Windows:
- Open Command Prompt
- Run: wmic os get osarchitecture
- Result should be "64-bit" (x64)

Linux:
- Run: uname -m
- Result should be "x86_64"

macOS:
- Run: uname -m
- Result is "x86_64" (Intel) or "arm64" (Apple Silicon)

## Step 2: Download Binary Package

Download the appropriate package from dist/binaries/:

Windows:
- dist/binaries/windows/x64/release/ (production)
- dist/binaries/windows/x64/debug/ (development)

Linux:
- dist/binaries/linux/x64/release/

macOS:
- dist/binaries/macos/x64/release/ (Intel)
- dist/binaries/macos/arm64/release/ (Apple Silicon)

Package Contents:
- bin/ - Executable programs
- lib/ - Library files (static and shared)
- include/ - Header files for development

## Step 3: Extract Package

### Windows (PowerShell)
```powershell
Expand-Archive -Path package.zip -DestinationPath C:\lyradb
```

### Windows (Command Prompt)
```cmd
tar -xf package.zip
```

### Linux/macOS
```bash
tar -xzf package.tar.gz
cd lyradb-1.0.0
```

## Step 4: Verify Installation

### Windows
```cmd
dir lib
dir bin
dir include
```

### Linux/macOS
```bash
ls -la lib/
ls -la bin/
ls -la include/
```

Expected files:
- lib/lyradb_formats.lib (Windows) or liblyradb_formats.a (Linux/macOS)
- Header files in include/lyradb/

## Step 5: Set Up Development Environment

### Windows (Visual Studio)

1. In your Visual Studio project:
   - Add include directory: Properties > VC++ Directories > Include Directories
   - Add lib directory: Properties > VC++ Directories > Library Directories
   - Link library: Linker > Input > Additional Dependencies > Add lyradb_formats.lib

2. Example in Visual Studio:
   ```
   Include Directories: C:\lyradb\include
   Library Directories: C:\lyradb\lib
   Additional Dependencies: lyradb_formats.lib
   ```

### Linux/macOS (GCC/Clang)

1. Compile with:
   ```bash
   g++ -I/path/to/include -L/path/to/lib -std=c++17 -O2 main.cpp -llyradb_formats -o main
   ```

2. Or set environment variables:
   ```bash
   export CPATH=/path/to/include:$CPATH
   export LIBRARY_PATH=/path/to/lib:$LIBRARY_PATH
   export LD_LIBRARY_PATH=/path/to/lib:$LD_LIBRARY_PATH
   ```

### CMake (All Platforms)

Add to CMakeLists.txt:
```cmake
include_directories(/path/to/include)
link_directories(/path/to/lib)
link_libraries(lyradb_formats)

add_executable(myapp main.cpp)
target_link_libraries(myapp lyradb_formats)
```

## Step 6: Create Test Program

Create test_install.cpp:

```cpp
#include "lyradb/lyradb_formats.h"
#include <iostream>

int main() {
    using namespace lyradb;
    
    // Test library availability
    LyraDBFormat db;
    db.database_name = "test";
    
    std::cout << "LyraDB library loaded successfully!" << std::endl;
    return 0;
}
```

## Step 7: Compile and Run

### Windows (MSVC)
```cmd
cl /std:c++17 /I"C:\lyradb\include" /EHsc /O2 test_install.cpp /link /LIBPATH:"C:\lyradb\lib" lyradb_formats.lib
test_install.exe
```

### Linux (GCC)
```bash
g++ -std=c++17 -I/path/to/include -O2 test_install.cpp -L/path/to/lib -llyradb_formats -o test_install
./test_install
```

### macOS (Clang)
```bash
clang++ -std=c++17 -I/path/to/include -O2 test_install.cpp -L/path/to/lib -llyradb_formats -o test_install
./test_install
```

Expected output:
```
LyraDB library loaded successfully!
```

If successful, the binary package is properly installed.

## Step 8: Integrate Into Your Project

Now you can use LyraDB in your project:

```cpp
#include "lyradb/lyradb_formats.h"

using namespace lyradb;

int main() {
    // Your application code here
    LyraDBFormat database;
    // ... use library
    return 0;
}
```

## File Structure Setup

Recommended project structure:

```
myproject/
├── src/
│   └── main.cpp
├── include/  (copy lyradb headers here)
├── lib/      (copy lyradb libraries here)
└── CMakeLists.txt
```

## Common Issues

### Library Not Found
- Verify lib/ directory location
- Check LD_LIBRARY_PATH is set correctly
- Ensure 64-bit library on 64-bit system

### Header File Not Found
- Verify include/ directory exists
- Check include path is set in compiler flags
- Ensure header files are readable

### Symbol Errors
- Verify header and library versions match
- Check C++ standard is set to C++17 or higher
- Use extern "C" only for C API

### Runtime Errors
- Check library dependencies (ldd on Linux)
- Verify runtime libraries are available
- See PLATFORM_NOTES.md for platform-specific info

## Next Steps

1. Review API documentation: docs/04_CPP_API_REFERENCE.md
2. Check examples: examples/ directory
3. See INSTALLATION.md for detailed instructions
4. Read docs/ for complete reference

## Alternative Installation Methods

If binary packages don't work for your system:

1. Conan Package
   ```bash
   conan install lyradb_formats/1.0.0
   ```

2. Build from Source
   ```bash
   git clone https://github.com/Seread335/LyraDB.git
   cd LyraDB
   build_windows.bat  # or ./build_linux.sh
   ```

## Support

For issues:
1. Check TROUBLESHOOTING.md
2. Review FAQ.md
3. See PLATFORM_NOTES.md
4. Open issue on GitHub

## Version Information

Current Version: 1.0.0
Release Date: December 2024
License: MIT

See dist/binaries/VERSION.txt for build details.
