# 📦 LyraDB Distribution Packages

## Contents

```
dist/
├── windows/
│   ├── release/           # Windows Release Build (Optimized)
│   │   ├── bin/          # Executables (test_formats.exe, usage_demo.exe, etc.)
│   │   ├── lib/          # Static library (lyradb_formats.lib)
│   │   └── include/      # Header files
│   │
│   └── debug/             # Windows Debug Build (With symbols)
│       ├── bin/          # Debug executables
│       ├── lib/          # Debug library (lyradb_formats.lib)
│       └── include/      # Header files
│
├── documentation/         # All guides and documentation
│   ├── INSTALLATION.md
│   ├── QUICK_START.md
│   ├── FAQ.md
│   ├── DISTRIBUTION_GUIDE.md
│   └── ... (all API references)
│
├── libraries/             # Pre-built libraries for distribution
│   ├── Windows/
│   ├── Linux/
│   └── macOS/
│
└── examples/              # Example projects
    ├── conan_usage_example/
    ├── simple_database.cpp
    └── format_demo.cpp
```

## Quick Installation (Choose One)

### ✅ Option 1: Use Conan (Recommended)
```bash
conan install lyradb_formats/1.0.0
```

### ✅ Option 2: Use Pre-built Binaries
1. Copy `windows/release/lib/lyradb_formats.lib` to your project
2. Copy `windows/release/include/` to your project
3. Link library in your build
4. Done!

### ✅ Option 3: Build from Source
```bash
# Go back to root LyraDB directory
cd ..
build_windows.bat
```

## Windows Release Contents

### `windows/release/bin/`
- `test_formats.exe` - Test all 3 file formats
- `usage_demo.exe` - Production usage examples
- `web_app.exe` - E-commerce demo application

### `windows/release/lib/`
- `lyradb_formats.lib` - Static library (production)
- `lyradb_formats.pdb` - Debug symbols (optional)

### `windows/release/include/`
- All header files needed for integration

## Windows Debug Contents

Same structure as Release but with:
- Debug symbols enabled
- Optimizations disabled
- Larger binary size
- Better debugging experience

## Documentation

Quick access to all guides:

| Document | Purpose |
|----------|---------|
| INSTALLATION.md | All installation options |
| QUICK_START.md | 5-minute tutorial |
| FAQ.md | Common questions answered |
| DISTRIBUTION_GUIDE.md | Distribution methods |

## Usage

### Include Header
```cpp
#include "lyradb/lyradb_formats.h"
```

### Link Library (Windows)
```
/link lyradb_formats.lib
```

### Compiler Settings
```
/std:c++17 /EHsc
```

## Verification

Verify installation with:
```cpp
#include "lyradb/lyradb_formats.h"
using namespace lyradb;

int main() {
    LyraDBFormat db;
    // If this compiles, you're ready!
    return 0;
}
```

## Support

- 📖 See `documentation/` for all guides
- 🐛 Report issues on GitHub
- 💬 Ask questions in Discussions

## License

MIT License - See LICENSE file in parent directory

---

**Version:** 1.0.0
**Status:** ✅ Production Ready
**Last Updated:** 2024-Q1
