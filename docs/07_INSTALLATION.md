# Installation Guide

## System Requirements

Before installing LyraDB, ensure your system meets these requirements:

Operating Systems:
- Windows 7 or later (32-bit or 64-bit)
- Linux (Ubuntu 16.04+, CentOS 7+, Debian 8+)
- macOS 10.10 or later

Compiler Requirements:
- GCC 4.8 or later (Linux/macOS)
- Clang 3.3 or later (Linux/macOS)
- Microsoft Visual C++ 2015 or later (Windows)

Build Tools:
- CMake 3.10 or later
- Make (Linux/macOS) or MSBuild (Windows)

Hardware:
- Memory: Minimum 512MB RAM (recommended 2GB+)
- Disk Space: 50MB for source and build artifacts
- Processor: Any 32-bit or 64-bit processor

---

## Download and Extraction

1. Obtain the LyraDB source code from the repository or distribution package

2. Extract the archive to a directory:

Windows:
```
C:\path\to\LyraDB
```

Linux/macOS:
```
~/LyraDB
/opt/lyradb
```

3. Verify the directory structure:

```
LyraDB/
  build/
  include/
  src/
  examples/
  tests/
  CMakeLists.txt
  README.md
  LICENSE
```

---

## Installation on Windows

### Step 1: Install Prerequisites

Visual Studio:
1. Download Visual Studio Community from https://visualstudio.microsoft.com/
2. Run the installer
3. Select "Desktop development with C++"
4. Complete installation

Or Visual Studio Build Tools:
1. Download from https://visualstudio.microsoft.com/downloads/
2. Select "Build Tools for Visual Studio"
3. Choose "Desktop development with C++" workload
4. Complete installation

CMake:
1. Download from https://cmake.org/download/
2. Run the installer
3. Select "Add CMake to the system PATH for all users"
4. Complete installation

### Step 2: Create Build Directory

```batch
cd C:\path\to\LyraDB
mkdir build
cd build
```

### Step 3: Configure Build

```batch
cmake .. -DCMAKE_BUILD_TYPE=Release -G "Visual Studio 16 2019" -A x64
```

Or for other Visual Studio versions:
```batch
cmake .. -G "Visual Studio 15 2017 Win64"
cmake .. -G "Visual Studio 17 2022" -A x64
```

### Step 4: Build Project

```batch
msbuild lyradb.sln /p:Configuration=Release /p:Platform=x64
```

Or use MSBuild directly:
```batch
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe" ^
    lyradb.sln /p:Configuration=Release /p:Platform=x64
```

### Step 5: Verify Installation

```batch
cd Release
comprehensive_test.exe
music_test.exe
```

Expected output:
- Tests run without errors
- All assertions pass
- Database operations complete successfully

---

## Installation on Linux

### Step 1: Install Prerequisites

Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install build-essential
sudo apt-get install cmake
sudo apt-get install git
```

CentOS/RHEL:
```bash
sudo yum groupinstall "Development Tools"
sudo yum install cmake
```

Fedora:
```bash
sudo dnf install gcc gcc-c++ make
sudo dnf install cmake
```

### Step 2: Create Build Directory

```bash
cd ~/LyraDB
mkdir build
cd build
```

### Step 3: Configure Build

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
```

Or with optimization:
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3"
```

### Step 4: Build Project

```bash
make -j$(nproc)
```

The -j option runs parallel builds using all available CPU cores.

Or with specific number of jobs:
```bash
make -j4
```

### Step 5: Verify Installation

```bash
./Release/comprehensive_test
./Release/music_test
```

### Step 6: Install to System (Optional)

```bash
sudo make install
```

This installs libraries to /usr/local/lib and headers to /usr/local/include.

---

## Installation on macOS

### Step 1: Install Prerequisites

Using Homebrew:
```bash
brew install cmake
brew install gcc
```

Or Xcode:
```bash
xcode-select --install
brew install cmake
```

### Step 2: Create Build Directory

```bash
cd ~/LyraDB
mkdir build
cd build
```

### Step 3: Configure Build

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
```

### Step 4: Build Project

```bash
make -j$(sysctl -n hw.ncpu)
```

### Step 5: Verify Installation

```bash
./Release/comprehensive_test
./Release/music_test
```

### Step 6: Install to System (Optional)

```bash
sudo make install
```

---

## Docker Installation

To run LyraDB in Docker:

1. Create Dockerfile:

```dockerfile
FROM ubuntu:20.04

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git

WORKDIR /app

COPY . /app/LyraDB

WORKDIR /app/LyraDB
RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j4

CMD ["./Release/music_test"]
```

2. Build Docker image:

```bash
docker build -t lyradb:1.2.0 .
```

3. Run container:

```bash
docker run -it lyradb:1.2.0
```

---

## CMake Configuration Options

Common CMake configuration flags:

Enable Debug Symbols:
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
```

Optimize for Performance:
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3"
```

Enable Position Independent Code (for shared libraries):
```bash
cmake .. -DCMAKE_CXX_FLAGS="-fPIC"
```

Specify Compiler:
```bash
cmake .. -DCMAKE_CXX_COMPILER=g++-11
```

Set Installation Prefix:
```bash
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/lyradb
```

---

## Compiler-Specific Installation

### GCC-based Systems

Linux Mint/Ubuntu:
```bash
sudo apt-get install g++
cmake .. -DCMAKE_CXX_COMPILER=g++
make
```

### Clang-based Systems

macOS with Clang:
```bash
cmake .. -DCMAKE_CXX_COMPILER=clang++
make
```

Linux with Clang:
```bash
sudo apt-get install clang
cmake .. -DCMAKE_CXX_COMPILER=clang++
make
```

### MSVC on Windows

Visual Studio 2022:
```batch
cmake .. -G "Visual Studio 17 2022" -A x64
msbuild lyradb.sln /p:Configuration=Release
```

Visual Studio 2019:
```batch
cmake .. -G "Visual Studio 16 2019" -A x64
msbuild lyradb.sln /p:Configuration=Release
```

---

## Building Specific Components

Build only the core library:
```bash
make lyradb_core
```

Build only C API:
```bash
make lyradb_c
```

Build only tests:
```bash
make RUN_TESTS
```

Build only examples:
```bash
make music_test
make comprehensive_test
```

---

## Custom Build Paths

### Out-of-Source Build (Recommended)

```bash
cd ~/
mkdir lyradb-build
cd lyradb-build
cmake ~/LyraDB -DCMAKE_BUILD_TYPE=Release
make
```

### In-Source Build (Not Recommended)

```bash
cd ~/LyraDB
cmake . -DCMAKE_BUILD_TYPE=Release
make
```

---

## Troubleshooting Installation

### CMake Not Found

Windows:
- Add CMake bin directory to PATH environment variable
- Restart terminal/IDE

Linux/macOS:
```bash
which cmake
```

If not found, install again: `apt-get install cmake` or `brew install cmake`

### Compiler Not Found

Verify compiler installation:
```bash
g++ --version
clang++ --version
cl.exe /?  (Windows)
```

### Build Fails

1. Clean previous build:
```bash
rm -rf build
mkdir build
cd build
```

2. Check CMake output for specific errors
3. Verify all prerequisites are installed
4. Check disk space: `df -h` (Linux/macOS)

### Permission Denied Error

Linux/macOS for system install:
```bash
sudo make install
```

Or use local installation:
```bash
cmake .. -DCMAKE_INSTALL_PREFIX=~/lyradb
make
make install
```

---

## Linking Against LyraDB

After installation, use LyraDB in your projects:

C++ projects:
```cpp
#include "lyradb/database.h"
#include "lyradb/sql_parser.h"

int main() {
    lyradb::Database db;
    // Use database
    return 0;
}
```

Compile and link:
```bash
g++ -std=c++11 -I/path/to/lyradb/include program.cpp \
    -L/path/to/lyradb/lib -llyradb_core -o program
```

C projects:
```c
#include "lyradb_c.h"

int main() {
    lyradb_handle_t db = lyradb_create_database();
    // Use database
    lyradb_destroy_database(db);
    return 0;
}
```

Compile and link:
```bash
gcc -I/path/to/lyradb/include program.c \
    -L/path/to/lyradb/lib -llyradb_c -o program
```

---

## Environment Configuration

Set library path for runtime (Linux):
```bash
export LD_LIBRARY_PATH=/path/to/lyradb/lib:$LD_LIBRARY_PATH
./program
```

Or permanently in shell profile (~/.bashrc or ~/.bash_profile):
```bash
export LD_LIBRARY_PATH=/path/to/lyradb/lib:$LD_LIBRARY_PATH
```

macOS:
```bash
export DYLD_LIBRARY_PATH=/path/to/lyradb/lib:$DYLD_LIBRARY_PATH
```

---

## Uninstallation

Remove system installation:

Linux:
```bash
sudo rm -rf /usr/local/include/lyradb*
sudo rm -rf /usr/local/lib/liblyradb*
```

macOS:
```bash
sudo rm -rf /usr/local/include/lyradb*
sudo rm -rf /usr/local/lib/liblyradb*
```

Remove build directory:
```bash
rm -rf ~/LyraDB/build
```

---

## Next Steps

After successful installation:

1. Read the Getting Started guide
2. Review example code in examples/ directory
3. Consult SQL Reference for statement syntax
4. Check API Reference for programming details
5. Run provided tests to verify functionality
