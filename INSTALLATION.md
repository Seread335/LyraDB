# 📖 LyraDB Installation Guide

## Quick Installation (3 Options)

### Option 1: Conan Package (Recommended - Easiest) ⭐

```bash
# Create conanfile.txt
[requires]
lyradb_formats/1.0.0

[generators]
CMakeDeps
CMakeToolchain

# Install
conan install .

# Use in CMakeLists.txt
find_package(lyradb_formats REQUIRED)
target_link_libraries(app lyradb_formats::lyradb_formats)
```

**Advantages:**
- ✅ No compilation needed
- ✅ Automatic dependency management
- ✅ Works on Windows, Linux, macOS
- ✅ One command: `conan install`

### Option 2: Source Code Build

#### Prerequisites
- **C++17 Compiler**
  - Windows: MSVC 19.4+ (Visual Studio 2022)
  - Linux: GCC 9+ or Clang 10+
  - macOS: Clang 10+
- **CMake 3.20+**

#### Windows Build
```bash
git clone https://github.com/Seread335/LyraDB.git
cd LyraDB
build_windows.bat
```

#### Linux Build
```bash
git clone https://github.com/Seread335/LyraDB.git
cd LyraDB
./build_linux.sh
```

#### macOS Build
```bash
git clone https://github.com/Seread335/LyraDB.git
cd LyraDB
./build_macos.sh
```

### Option 3: Pre-built Binaries

Check `dist/` directory for pre-compiled libraries:
- `dist/windows/release/` - Windows Release build
- `dist/windows/debug/` - Windows Debug build
- `dist/documentation/` - All documentation

## Integration Paths

### Path 1: Using Conan (Fastest)
```
1. conan install lyradb_formats/1.0.0
2. Add to conanfile.txt
3. Use headers in code
4. Done! ✓
```

### Path 2: Using CMake with Find Module
```bash
# Copy to your CMake modules
find_package(lyradb_formats REQUIRED)

# Link in your target
target_link_libraries(myapp lyradb_formats::lyradb_formats)
```

### Path 3: Manual Integration
```cpp
// Include header
#include "lyradb/lyradb_formats.h"

// Use namespace
using namespace lyradb;

// Link against liblyradb_formats
```

## File Structure After Installation

```
your_project/
├── include/
│   └── lyradb/
│       └── lyradb_formats.h
├── lib/
│   ├── lyradb_formats.lib (Windows static)
│   ├── liblyradb_formats.a (Linux/macOS static)
│   └── liblyradb_formats.so (Linux shared)
└── src/
    └── lyradb_formats.cpp
```

## Library Features

### 3 File Formats Included
| Format | Extension | Purpose |
|--------|-----------|---------|
| Database | `.lyradb` | Database snapshots with full metadata |
| Iterator | `.lyradbite` | Sequential data access cursors |
| Archive | `.lyra` | Encrypted backups with versioning |

### APIs Available
- ✅ C++ API (C++17)
- ✅ C API (C99 compatible)
- ✅ REST API (via server)

## Compilation Flags

### Windows (MSVC)
```bash
cl /std:c++17 /I"path/to/include" /EHsc /O2 main.cpp lyradb_formats.cpp
```

### Linux/macOS (GCC/Clang)
```bash
g++ -std=c++17 -I"path/to/include" -O2 main.cpp lyradb_formats.cpp -o app
```

## Verification

After installation, verify setup:

```cpp
#include "lyradb/lyradb_formats.h"
#include <iostream>

int main() {
    using namespace lyradb;
    std::cout << "LyraDB ready!" << std::endl;
    return 0;
}
```

Compile and run - if successful, installation is complete! ✓

## Troubleshooting

### "lyradb_formats not found"
```bash
# Make sure Conan package is created
conan create <lyradb-path>

# Or use pre-built from dist/
```

### Build errors with C++17
```bash
# Ensure C++17 flag is set
conan install . -s compiler.cppstd=17
```

### Include path issues
```bash
# Use absolute paths or proper CMake configuration
-I"C:\Path\To\include\lyradb"
```

## Support & Documentation

- 📖 **Full Docs:** See `docs/` folder
- 💬 **Issues:** https://github.com/Seread335/LyraDB/issues
- 🔗 **Repository:** https://github.com/Seread335/LyraDB

## Next Steps

1. Choose installation method above
2. Follow your chosen path
3. Check examples in `examples/conan_usage_example/`
4. Read API documentation in `docs/`
5. Start building! 🚀

---

**Happy building with LyraDB!**
