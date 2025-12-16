/* All public headers */
#pragma once

#include <cstdint>
#include <string>
#include "config.h"
#include "version.h"
#include "data_types.h"
#include "schema.h"
#include "column.h"
#include "table.h"
#include "database.h"
#include "compression.h"
#include "buffer_manager.h"
#include "lru2.h"
#include "query_result.h"
#include "query_plan.h"
#include "query_executor.h"
#include "sql_lexer.h"
#include "sql_parser.h"
#include "expression_evaluator.h"
#include "query_execution_engine.h"
#include "zone_map.h"
#include "bloom_filter.h"
#include "b_tree_index.h"
#include "hash_index.h"
#include "bitmap_index.h"
#include "index_manager.h"
#include "index_aware_optimizer.h"

/**
 * @brief LyraDB - Modern Embeddable Columnar Database
 * 
 * Main namespace for all LyraDB functionality
 */
namespace lyradb {

// Convenience type aliases
using i32 = int32_t;
using i64 = int64_t;
using f32 = float;
using f64 = double;
using str = std::string;

} // namespace lyradb
