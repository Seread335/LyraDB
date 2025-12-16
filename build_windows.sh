#!/bin/bash
# Build script for Windows (MSVC)

set -e

echo "📦 Building LyraDB for Windows..."
echo ""

BUILD_TYPE=${1:-Release}
ARCH=${2:-x64}

# Create build directory
mkdir -p build_windows_$ARCH
cd build_windows_$ARCH

# Generate project
echo "🔨 Generating CMake project..."
cmake .. -G "Visual Studio 17 2022" -A $ARCH \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DBUILD_REST_SERVER=ON \
    -DBUILD_TESTS=ON \
    -DENABLE_SIMD=ON

# Build
echo "🔨 Building..."
cmake --build . --config $BUILD_TYPE -j$(nproc)

# Package
echo "📦 Creating distribution package..."
mkdir -p ../dist/windows_$ARCH
cp $BUILD_TYPE/lyradb.dll ../dist/windows_$ARCH/
cp $BUILD_TYPE/lyradb.lib ../dist/windows_$ARCH/
cp ../include/lyradb_c.h ../dist/windows_$ARCH/
cp ../include/lyradb.h ../dist/windows_$ARCH/ 2>/dev/null || true

echo ""
echo "✅ Windows build complete!"
echo "   Output: dist/windows_$ARCH/"
echo "   - lyradb.dll (shared library)"
echo "   - lyradb.lib (import library)"
echo "   - lyradb_c.h (C API header)"
echo ""
