# LyraDB Pre-compiled Binaries

This directory contains pre-compiled binary distributions of LyraDB for all major platforms.

## Supported Platforms

### Windows (x64)
- Release Build: Optimized for production deployment
- Debug Build: Contains debugging symbols for development

System Requirements:
- Windows 7 or later
- Visual C++ Runtime 14.0 or later (usually pre-installed)

### Linux (x64)
- Release Build: Optimized for production deployment
- Requires glibc 2.17 or later (Ubuntu 14.04+, CentOS 7+, Debian 8+)

### macOS
- x64: Intel processors
- ARM64: Apple Silicon (M1, M2, M3)
- Both include release builds optimized for the target architecture

## Directory Structure

```
binaries/
├── windows/
│   └── x64/
│       ├── release/
│       │   ├── bin/     Executables
│       │   └── lib/     Library files
│       └── debug/
│           ├── bin/     Executables with debug symbols
│           └── lib/     Library files with debug info
├── linux/
│   └── x64/
│       └── release/
│           ├── bin/     Executables
│           └── lib/     Static and shared libraries
└── macos/
    ├── x64/
    │   └── release/
    │       ├── bin/     Executables
    │       └── lib/     Library files
    └── arm64/
        └── release/
            ├── bin/     Executables
            └── lib/     Library files
```

## Installation

### Windows

1. Download the Windows x64 release or debug package
2. Extract to your preferred location
3. Add the bin/ directory to your PATH environment variable
4. Link against lib/lyradb_formats.lib in your projects

### Linux

1. Download the Linux x64 release package
2. Extract to your preferred location
3. Add lib/ to your LD_LIBRARY_PATH
4. Link against liblyradb_formats.a or liblyradb_formats.so

### macOS

1. Download the appropriate package (x64 or ARM64)
2. Extract to your preferred location
3. Add lib/ to your DYLD_LIBRARY_PATH
4. Link against liblyradb_formats.a or liblyradb_formats.dylib

## Build Information

All binaries are built with:
- C++17 standard
- Full optimization (-O2 / /O2)
- Position-independent code
- Stripped symbols (release builds)

Build dates and compiler versions are documented in each package.

## Header Files

Header files for development are available in the include/ directory of each distribution package.

Required headers for integration:
- lyradb/lyradb_formats.h - Main library header
- lyradb/lyradb.h - Database core
- lyradb_c.h - C API bindings

## Verification

After extracting, verify the installation by checking:
1. Header files exist in include/
2. Library files exist in lib/
3. Executable files exist in bin/
4. File permissions are preserved

On Unix-based systems (Linux, macOS), library files should be readable and linkable.

## Alternative Installation Methods

If you prefer not to use pre-compiled binaries:

1. Conan Package - Recommended for cross-platform projects
   ```bash
   conan install lyradb_formats/1.0.0
   ```

2. Build from Source - Full control over compilation
   ```bash
   git clone https://github.com/Seread335/LyraDB.git
   cd LyraDB
   build_windows.bat  # Windows
   ./build_linux.sh   # Linux
   ./build_macos.sh   # macOS
   ```

## Support

For issues with binary packages:
1. Check INSTALLATION.md for detailed setup instructions
2. Review FAQ.md for common questions
3. See docs/ for API documentation
4. File an issue on GitHub if problems persist

## License

All binaries are distributed under the MIT License. See LICENSE file for terms.

## Size Information

Pre-compiled binaries are optimized for size while maintaining performance. Typical sizes:

- Windows Release: ~1.4 MB (library)
- Windows Debug: ~4.5 MB (with symbols)
- Linux Release: ~1.2 MB
- macOS Release: ~1.3 MB (x64 or ARM64)

## Update Information

Binaries are updated with each release. Check the VERSION file in each package to confirm your version.

Current version: 1.0.0
Release date: December 2024
