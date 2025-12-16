#!/bin/bash
# Universal build script for all platforms

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║     LyraDB Embedded Library Build System                       ║"
echo "║     Universal Multi-Platform Distribution Builder              ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

# Detect OS
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" ]]; then
    OS="Windows"
    BUILD_SCRIPT="build_windows.sh"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OS="macOS"
    BUILD_SCRIPT="build_macos.sh"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS="Linux"
    BUILD_SCRIPT="build_linux.sh"
else
    OS="Unknown"
    echo "❌ Unsupported OS: $OSTYPE"
    exit 1
fi

BUILD_TYPE=${1:-Release}

echo "🖥️  Detected OS: $OS"
echo "🏗️  Build Type: $BUILD_TYPE"
echo ""

if [ "$1" = "--all" ] || [ "$1" = "all" ]; then
    echo "⚠️  Building for ALL platforms requires:"
    echo "   - Windows build environment (Visual Studio 2022+)"
    echo "   - Linux build environment (GCC/Clang)"
    echo "   - macOS build environment (Xcode)"
    echo ""
    echo "This script will only build for the current OS."
    echo "Run on each OS separately to generate all distributions."
    echo ""
fi

if [ ! -f "$BUILD_SCRIPT" ]; then
    echo "❌ Build script not found: $BUILD_SCRIPT"
    exit 1
fi

echo "📋 Starting build process..."
bash "$BUILD_SCRIPT" "$BUILD_TYPE"

echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║     Build Complete                                             ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""
echo "📦 Distribution artifacts:"
ls -lR dist/ 2>/dev/null || echo "   (run on each OS to generate distributions)"
echo ""
