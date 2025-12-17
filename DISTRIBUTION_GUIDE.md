# 📦 LyraDB Distribution Guide

## Distribution Options

### 🍎 Option 1: Conan Package (Recommended)

**Latest Version:** `lyradb_formats/1.0.0`

```bash
# Install
conan install lyradb_formats/1.0.0

# Or in conanfile.txt
[requires]
lyradb_formats/1.0.0
```

**Advantages:**
- ✅ Works on all OS platforms
- ✅ No compilation needed
- ✅ Automatic dependency management
- ✅ Easy version management
- ✅ CI/CD friendly

**Location:** Conan Central Repository
**Repository:** https://github.com/Seread335/LyraDB

---

### 📦 Option 2: Pre-Built Binaries

Located in `/dist/` folder:

```
dist/
├── windows/
│   ├── release/          # Windows Release build
│   │   ├── bin/         # Executables
│   │   ├── lib/         # Libraries
│   │   └── include/     # Headers
│   └── debug/            # Windows Debug build
├── documentation/        # All guides & docs
├── libraries/            # Static/shared libs
└── examples/             # Example projects
```

**Windows Release Package Contents:**
- ✅ `lyradb_formats.lib` - Static library
- ✅ `lyradb_formats.dll` - Dynamic library (if applicable)
- ✅ Header files (include/)
- ✅ Example projects
- ✅ Documentation
- ✅ Build scripts

---

### 💾 Option 3: Source Code

**Repository:** https://github.com/Seread335/LyraDB

```bash
git clone https://github.com/Seread335/LyraDB.git
cd LyraDB

# Build scripts available for all platforms
./build_windows.bat    # Windows
./build_linux.sh       # Linux
./build_macos.sh       # macOS
```

**Advantages:**
- ✅ Full source code access
- ✅ Build customization
- ✅ Contribution ready
- ✅ Latest development version

---

## Windows Installation Paths

### Path 1: Using Conan (Fastest)
```
1. Create conanfile.txt with lyradb_formats/1.0.0
2. Run: conan install .
3. Done!
```
**Time:** 2-3 minutes | **Skills:** Basic

### Path 2: Using Pre-Built Binaries
```
1. Download from dist/windows/release/
2. Copy includes to your project
3. Link against .lib file
4. Done!
```
**Time:** 5 minutes | **Skills:** Basic

### Path 3: Build from Source
```
1. Clone repository
2. Run: build_windows.bat
3. CMake configures & builds
4. Use from build/ directory
```
**Time:** 10-15 minutes | **Skills:** Intermediate

---

## Linux Installation Paths

### Path 1: Conan Package
```bash
conan install lyradb_formats/1.0.0
cmake .
make
```

### Path 2: Build from Source
```bash
git clone https://github.com/Seread335/LyraDB.git
cd LyraDB
./build_linux.sh
```

**Supports:**
- ✅ Ubuntu 20.04+
- ✅ CentOS 8+
- ✅ Debian 11+
- ✅ Any Linux with GCC 9+

---

## macOS Installation Paths

### Path 1: Conan Package
```bash
conan install lyradb_formats/1.0.0
cmake .
make
```

### Path 2: Build from Source
```bash
git clone https://github.com/Seread335/LyraDB.git
cd LyraDB
./build_macos.sh
```

**Supports:**
- ✅ macOS 11+
- ✅ Apple Silicon (M1/M2/M3)
- ✅ Intel x86_64
- ✅ Clang 10+

---

## File Format Support

All distributions include support for 3 file formats:

| Format | Extension | Features |
|--------|-----------|----------|
| Database | `.lyradb` | Full schema, tables, indexes, metadata |
| Iterator | `.lyradbite` | Cursor position, column info, pagination |
| Archive | `.lyra` | Encryption, compression, versioning, integrity |

All formats include:
- ✅ CRC64 checksums
- ✅ Magic signatures
- ✅ Version support
- ✅ Error detection

---

## System Requirements by Distribution

### Conan Package
- Internet connection (first install)
- Conan 2.0+
- Supported compiler (any C++17 capable)

### Pre-Built Binaries (Windows)
- Windows 7+
- Visual C++ Runtime (included)
- 20 MB disk space

### Source Build (All Platforms)
- C++17 compiler (GCC 9+, Clang 10+, MSVC 19.4+)
- CMake 3.20+
- 200 MB disk space (source + build)

---

## Verification Steps

### After Conan Installation
```cpp
#include "lyradb/lyradb_formats.h"
using namespace lyradb;

int main() {
    LyraDBFormat db;
    db.database_name = "test";
    // If compiles = Success!
}
```

### After Pre-Built Installation
```cpp
// Link against:
// Windows: lyradb_formats.lib
// Linux: liblyradb_formats.a
// macOS: liblyradb_formats.a
```

### After Source Build
```bash
cd build
./Release/test_formats.exe  # Windows
./Release/test_formats      # Linux/macOS
```

---

## Release Timeline

| Version | Date | Notes |
|---------|------|-------|
| 1.0.0 | 2024-Q1 | Initial release with 3 formats |
| 1.1.0 | Planned | Additional compression algorithms |
| 1.2.0 | Planned | REST API enhancements |
| 2.0.0 | Planned | Distributed support |

---

## Support Channels

| Channel | Purpose |
|---------|---------|
| 📖 [Installation Guide](INSTALLATION.md) | Getting started |
| 📚 [Documentation](docs/) | API reference |
| 🐛 [Issues](https://github.com/Seread335/LyraDB/issues) | Bug reports |
| 💬 [Discussions](https://github.com/Seread335/LyraDB/discussions) | Questions |

---

## License

All distributions under MIT License - See LICENSE file

---

**Last Updated:** 2024-Q1
**Current Version:** 1.0.0
**Status:** ✅ Production Ready
