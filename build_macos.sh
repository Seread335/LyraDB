#!/bin/bash
# Build script for macOS (Clang)

set -e

echo "📦 Building LyraDB for macOS..."
echo ""

BUILD_TYPE=${1:-Release}

# Detect architecture
ARCH=$(uname -m)

# Create build directory
mkdir -p build_macos_$ARCH
cd build_macos_$ARCH

# Generate project
echo "🔨 Generating CMake project..."
cmake .. \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DBUILD_REST_SERVER=ON \
    -DBUILD_TESTS=ON \
    -DENABLE_SIMD=ON \
    -DCMAKE_OSX_ARCHITECTURES=$ARCH

# Build
echo "🔨 Building..."
make -j$(sysctl -n hw.ncpu)

# Package
echo "📦 Creating distribution package..."
mkdir -p ../dist/macos_$ARCH
cp liblyradb.dylib ../dist/macos_$ARCH/
cp liblyradb_static.a ../dist/macos_$ARCH/ 2>/dev/null || true
cp ../include/lyradb_c.h ../dist/macos_$ARCH/
cp ../include/lyradb.h ../dist/macos_$ARCH/ 2>/dev/null || true

# Set compatibility info
install_name_tool -id "@rpath/liblyradb.dylib" ../dist/macos_$ARCH/liblyradb.dylib || true

echo ""
echo "✅ macOS build complete!"
echo "   Output: dist/macos_$ARCH/"
echo "   - liblyradb.dylib (shared library)"
echo "   - liblyradb_static.a (static library)"
echo "   - lyradb_c.h (C API header)"
echo ""
