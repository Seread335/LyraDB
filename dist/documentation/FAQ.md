# ❓ Frequently Asked Questions (FAQ)

## Installation & Setup

### Q1: Which installation method should I use?
**A:** For most users:
- **Conan** → Easiest, recommended, 5 minutes
- **Pre-built** → Quick, no setup needed
- **Source** → Full control, compile customization

See [INSTALLATION.md](INSTALLATION.md) for detailed comparison.

### Q2: Do I need to compile LyraDB?
**A:** No! Unless you:
- Want to modify source code
- Need specific optimizations
- Are developing on unsupported platform

Use Conan or pre-built binaries instead.

### Q3: What are the system requirements?
**A:**
- **Windows:** Windows 7+ with 20 MB disk
- **Linux:** GCC 9+, CMake 3.20+
- **macOS:** Clang 10+, macOS 11+
- **All:** C++17 compiler

### Q4: Is Visual Studio required?
**A:** No. MSVC compiler works, but you can use:
- Visual Studio Community (free)
- GCC on WSL
- Clang on any platform
- MinGW

### Q5: Can I use Conan on Windows?
**A:** Yes! Install Conan, then:
```bash
conan install lyradb_formats/1.0.0
```

Works with MSVC, MinGW, and Clang.

---

## File Formats

### Q6: What's the difference between .lyradb, .lyradbite, and .lyra?

| Format | Use Case |
|--------|----------|
| `.lyradb` | Complete database snapshot - schemas, tables, indexes |
| `.lyradbite` | Streaming/iterating through data rows |
| `.lyra` | Encrypted backups and archives |

### Q7: Can I convert between formats?
**A:** Not directly. Design your files for the right purpose from the start:
- Need full database → Use `.lyradb`
- Iterating rows → Use `.lyradbite`
- Backup/encrypt → Use `.lyra`

### Q8: Are the files portable across platforms?
**A:** Yes! Binary format is platform-independent:
- Create on Windows
- Read on Linux
- Both work identically

### Q9: What's the maximum file size?
**A:** 64-bit size field supports:
- ~16 Exabytes maximum
- Practical limit: Disk space

### Q10: Do files include compression?
**A:** Yes, `.lyra` archives support:
- ZSTD compression (default)
- Automatic compression detection
- Custom compression settings

---

## Compilation & Building

### Q11: How do I use it in my CMake project?

```cmake
# Option 1: With Conan
find_package(lyradb_formats REQUIRED)
target_link_libraries(myapp lyradb_formats::lyradb_formats)

# Option 2: Without Conan
target_include_directories(myapp PRIVATE "path/to/include")
target_link_libraries(myapp "path/to/lyradb_formats.lib")
```

### Q12: What C++ version is required?
**A:** C++17 minimum. Compiler requirements:
- MSVC 19.4+ (Visual Studio 2022)
- GCC 9+
- Clang 10+

Set flag: `/std:c++17` (MSVC) or `-std=c++17` (GCC/Clang)

### Q13: Can I use it with an older compiler?
**A:** No. C++17 features are used throughout:
- Filesystem library
- Optional types
- Structured bindings

Upgrade compiler for compatibility.

### Q14: Does it work with -Wall -Wextra?
**A:** Yes! No warnings with strict flags.

### Q15: Can I use it in a header-only project?
**A:** No. It's a compiled library. Link against:
- Windows: `lyradb_formats.lib`
- Linux: `liblyradb_formats.a`
- macOS: `liblyradb_formats.a`

---

## Usage & Integration

### Q16: How do I read a .lyradb file?

```cpp
#include "lyradb/lyradb_formats.h"
using namespace lyradb;

LyraDBFormat db;
if (db.ReadFromFile("mydb.lyradb")) {
    // File loaded successfully
    std::cout << "Database: " << db.database_name << std::endl;
}
```

### Q17: How do I write a .lyra archive?

```cpp
LyraArchiveFormat archive;
archive.encryption_enabled = true;
archive.archive_name = "Backup";
archive.WriteToFile("backup.lyra");
```

### Q18: Can I modify files after creating them?
**A:** Currently: Read or write, not both.

Suggested workflow:
1. Read file into memory
2. Modify in memory
3. Write to new file
4. Replace original

### Q19: Are files thread-safe?
**A:** Use separate instances per thread:
```cpp
// Safe: each thread has own instance
thread1_db.WriteToFile("db1.lyradb");
thread2_db.WriteToFile("db2.lyradb");

// Unsafe: shared instance
shared_db.WriteToFile("db.lyradb");  // Race condition!
```

### Q20: Can multiple processes access the same file?
**A:** Not simultaneously. Use locking:
```cpp
// Process 1: Write lock
std::ofstream lock("db.lock");

// Process 2: Wait for lock release
std::ifstream check("db.lock");
```

---

## Performance & Optimization

### Q21: How large can files be?
**A:** Practical limits:
- Single file: Disk space
- Memory: File loaded into memory
- Typical: Up to 10 GB tested

### Q22: Is there a performance penalty for encryption?
**A:** Minimal (~5-10% on typical data):
- Encryption is efficient
- Trade: Small performance for strong security
- Acceptable for most use cases

### Q23: Which compression is fastest?
**A:** Speed hierarchy (fastest → slowest):
1. No compression
2. RLE (best for repetitive data)
3. Dictionary
4. Bitpacking
5. ZSTD (best compression ratio)

### Q24: Can I disable compression?
**A:** Yes, in `.lyra` archives:
```cpp
archive.compression_type = "none";
archive.WriteToFile("uncompressed.lyra");
```

### Q25: How do I profile my code?
**A:** LyraDB has minimal overhead. Profile your code:
- File I/O dominates typically
- Use Windows ETW or Linux perf
- Add timing around file operations

---

## Troubleshooting

### Q26: "lyradb/lyradb_formats.h not found"

**Solution:**
1. Check include path is correct
2. Verify header exists in `include/lyradb/`
3. Add to compiler: `-I"path/to/include"`

### Q27: Linker error "lyradb_formats not found"

**Solution:**
1. Link against library file:
   - Windows: `lyradb_formats.lib`
   - Linux: `liblyradb_formats.a`
2. Check library path
3. Add to linker: `-L"path/to/lib"`

### Q28: "File write failed"

**Solution:**
1. Check disk space
2. Check write permissions
3. Verify directory exists
4. Check filename for invalid characters

### Q29: File corruption detected

**Solution:**
1. Check disk health (chkdsk)
2. Verify file wasn't modified externally
3. Use backup copy
4. Report issue with corrupted file

### Q30: Conan package not found

**Solution:**
```bash
# Refresh Conan cache
conan remove '*' -f

# Reinstall package
conan install lyradb_formats/1.0.0

# Or build locally
conan create .
```

---

## Development & Contributing

### Q31: Where's the source code?
**A:** GitHub: https://github.com/Seread335/LyraDB

### Q32: How do I report bugs?
**A:** 
1. Check [ISSUES_FIXED.md](ISSUES_FIXED.md) for known issues
2. Search existing [issues](https://github.com/Seread335/LyraDB/issues)
3. Create new issue with:
   - Reproduction steps
   - Expected vs actual behavior
   - Your environment (OS, compiler, version)

### Q33: Can I contribute?
**A:** Yes! Pull requests welcome for:
- Bug fixes
- Feature additions
- Documentation improvements
- Example projects

### Q34: What's the roadmap?
**A:** Upcoming features:
- v1.1: Additional compression algorithms
- v1.2: REST API enhancements  
- v2.0: Distributed database support

See [README.md](README.md) for timeline.

### Q35: Is there a Slack or Discord?
**A:** Not yet. Use:
- GitHub Issues for bugs
- GitHub Discussions for questions
- Email for direct contact

---

## Licensing & Legal

### Q36: What license is LyraDB under?
**A:** MIT License - see [LICENSE](LICENSE) file

**Summary:**
- ✅ Use commercially
- ✅ Modify code
- ✅ Distribute
- ✅ Include in proprietary software
- ℹ️ Include license text
- ℹ️ No liability

### Q37: Can I use it in commercial software?
**A:** Yes! MIT license permits commercial use.

### Q38: Do I need to open-source my code?
**A:** No. MIT doesn't require source disclosure.

### Q39: Can I remove the license notice?
**A:** No. You must include MIT license text.

### Q40: What about patents?
**A:** No patent rights reserved. Full freedom to use.

---

## Resources & Support

### Need More Help?
- 📖 **[Installation Guide](INSTALLATION.md)** - Setup instructions
- 🚀 **[Quick Start](QUICK_START.md)** - 5-minute tutorial
- 📚 **[docs/](docs/)** - Full API documentation
- 🐛 **[Issues](https://github.com/Seread335/LyraDB/issues)** - Bug reports
- 💬 **[Discussions](https://github.com/Seread335/LyraDB/discussions)** - Questions

---

**Last Updated:** 2024-Q1
**LyraDB Version:** 1.0.0
**Status:** ✅ Production Ready

**Still have questions?** Open an issue on GitHub!
