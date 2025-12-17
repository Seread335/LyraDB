# Platform-Specific Build Notes

## Windows (x64)

### System Requirements
- Windows 7 SP1 or later
- Visual C++ Redistributable 2015-2022
- 512 MB RAM minimum, 2 GB recommended

### Installation Steps
1. Extract the distribution package
2. Copy header files to your include directory
3. Link against lib/lyradb_formats.lib
4. Set include path to find lyradb/lyradb_formats.h

### Compilation with MSVC
```
cl /std:c++17 /I"path/to/include" /EHsc /O2 program.cpp /link lyradb_formats.lib
```

### Runtime Dependencies
- VCRUNTIME140.dll
- MSVCP140.dll

Both are included in Windows 10+ or available from Microsoft downloads.

### Debugging
Debug build includes full symbols. Load .pdb files in Visual Studio:
- Program.exe
- Program.pdb

## Linux (x64)

### System Requirements
- Linux kernel 3.10 or later
- glibc 2.17 or later (Ubuntu 14.04+, CentOS 7+, Debian 8+)
- 512 MB RAM minimum, 2 GB recommended

### Supported Distributions
- Ubuntu 14.04 LTS and later
- CentOS 7 and later
- Debian 8 and later
- Fedora 20 and later
- Any distribution with matching glibc version

### Installation Steps
1. Extract the distribution package
2. Copy header files to /usr/local/include or project directory
3. Copy libraries to /usr/local/lib or project directory
4. Update LD_LIBRARY_PATH if using shared libraries

### Compilation with GCC
```bash
g++ -std=c++17 -I"path/to/include" -O2 program.cpp -L"path/to/lib" -llyradb_formats -o program
```

### Runtime Setup
For shared libraries, set:
```bash
export LD_LIBRARY_PATH="/path/to/lib:$LD_LIBRARY_PATH"
```

For static libraries, no runtime setup needed.

### Verification
```bash
ldd ./program  # Check dependencies
file ./program  # Verify architecture
```

## macOS (x64 and ARM64)

### System Requirements
- macOS 10.13 (High Sierra) or later for x64
- macOS 11.0 (Big Sur) or later for ARM64
- 512 MB RAM minimum, 2 GB recommended

### x64 (Intel Processors)
Works on Intel-based Mac computers with at least High Sierra.

### ARM64 (Apple Silicon)
Optimized for M1, M2, M3, and later chips.
x64 binaries run under Rosetta 2 emulation but performance is reduced.

### Installation Steps
1. Extract the distribution package
2. Copy headers to /usr/local/include or project directory
3. Copy libraries to /usr/local/lib or project directory
4. Update install_name_tool if relocating libraries

### Compilation with Clang
```bash
clang++ -std=c++17 -I"path/to/include" -O2 program.cpp -L"path/to/lib" -llyradb_formats -o program
```

### Runtime Setup
For shared libraries, update:
```bash
install_name_tool -change @loader_path liblyradb_formats.dylib
```

### Verification
```bash
file ./program  # Show architecture (x86_64 or arm64)
ldd ./program   # Check dependencies
```

## Cross-Compilation Notes

### Windows to Linux
Requires Linux cross-compiler toolchain. Set up:
- i686-w64-mingw32 (for 32-bit)
- x86_64-w64-mingw32 (for 64-bit)

### Linux to Windows
Requires mingw-w64 toolchain. Example:
```bash
x86_64-w64-mingw32-g++ -std=c++17 program.cpp -o program.exe
```

### macOS Cross-Compilation
macOS to other platforms requires full toolchain installation.
Most developers use CI/CD systems for cross-platform builds.

## Virtual Machines and Containers

### Docker Usage
All binaries work within Docker containers:

```dockerfile
# Linux
FROM ubuntu:20.04
COPY dist/binaries/linux/x64/release /opt/lyradb

# Windows
FROM mcr.microsoft.com/windows/servercore:ltsc2019
COPY dist/binaries/windows/x64/release C:\lyradb
```

### Virtual Machines
All binaries function identically in VirtualBox, VMware, Hyper-V, and other hypervisors when the host OS is supported.

## Architecture-Specific Notes

### 32-bit Support
32-bit binaries are not provided. For 32-bit systems, build from source or use Conan.

### ARM (Raspberry Pi, etc.)
ARM32 and ARM64 (non-Apple) builds are not provided. Use Conan or build from source.

### Other Architectures
PowerPC, MIPS, and other architectures require building from source.

## Troubleshooting

### Library Not Found
- Verify LD_LIBRARY_PATH (Linux) or DYLD_LIBRARY_PATH (macOS)
- Use ldd (Linux) or otool -L (macOS) to check dependencies
- Ensure 64-bit libraries on 64-bit systems

### Symbol Resolution Errors
- Ensure header files match library version
- Check C++ name mangling matches (extern "C" for C API)
- Verify compiler C++ standard is at least C++17

### Performance Issues
- Debug builds are slower than release builds
- Verify release build is being used for production
- Check system resource availability

## Support Resources

For platform-specific issues:
1. Platform documentation in docs/ directory
2. TROUBLESHOOTING.md for common problems
3. FAQ.md for frequently asked questions
4. GitHub issues for bug reports
