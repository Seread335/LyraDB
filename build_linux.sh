#!/bin/bash
# Build script for Linux (GCC/Clang)

set -e

echo "📦 Building LyraDB for Linux..."
echo ""

BUILD_TYPE=${1:-Release}
COMPILER=${2:-gcc}

# Detect architecture
ARCH=$(uname -m)

# Create build directory
mkdir -p build_linux_$ARCH
cd build_linux_$ARCH

# Set compiler
if [ "$COMPILER" = "clang" ]; then
    export CC=clang
    export CXX=clang++
else
    export CC=gcc
    export CXX=g++
fi

# Generate project
echo "🔨 Generating CMake project..."
cmake .. \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DBUILD_REST_SERVER=ON \
    -DBUILD_TESTS=ON \
    -DENABLE_SIMD=ON \
    -DCMAKE_C_COMPILER=$CC \
    -DCMAKE_CXX_COMPILER=$CXX

# Build
echo "🔨 Building..."
make -j$(nproc)

# Package
echo "📦 Creating distribution package..."
mkdir -p ../dist/linux_$ARCH
cp liblyradb.so ../dist/linux_$ARCH/
cp liblyradb_static.a ../dist/linux_$ARCH/ 2>/dev/null || true
cp ../include/lyradb_c.h ../dist/linux_$ARCH/
cp ../include/lyradb.h ../dist/linux_$ARCH/ 2>/dev/null || true

echo ""
echo "✅ Linux build complete!"
echo "   Output: dist/linux_$ARCH/"
echo "   - liblyradb.so (shared library)"
echo "   - liblyradb_static.a (static library)"
echo "   - lyradb_c.h (C API header)"
echo ""
