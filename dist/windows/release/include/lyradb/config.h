/* Configuration Header */
#pragma once

#define LYRADB_VERSION_MAJOR 0
#define LYRADB_VERSION_MINOR 1
#define LYRADB_VERSION_PATCH 0

// Feature flags
#define LYRADB_ENABLE_SIMD 1
#define LYRADB_ENABLE_COMPRESSION 1
#define LYRADB_ENABLE_PYTHON 1

// Performance tuning
#define LYRADB_DEFAULT_PAGE_SIZE (65536)        // 64 KB
#define LYRADB_DEFAULT_BATCH_SIZE (1024)        // Vector batch size
#define LYRADB_BUFFER_POOL_SIZE (1073741824)    // 1 GB

// Constants
#define LYRADB_MAX_COLUMNS (1024)
#define LYRADB_MAX_STRING_LENGTH (1048576)      // 1 MB
#define LYRADB_MAX_DECIMAL_PRECISION (38)
