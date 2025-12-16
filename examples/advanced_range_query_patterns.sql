/**
 * @file advanced_range_query_example.sql
 * @brief Advanced examples of B-tree range query optimization
 * 
 * Phase 4.2: B-Tree Query Optimization Patterns
 * Shows best practices for range queries on B-tree indexed columns
 */

-- ============================================================================
-- Example 1: Simple Range Query (Best Case for B-tree)
-- ============================================================================

-- Create index for optimization
CREATE INDEX idx_product_price ON products(price);

-- Query: Find products in medium price range
-- B-tree is PERFECT for this (low selectivity)
SELECT id, name, price FROM products
WHERE price > 50.0 AND price < 100.0
ORDER BY price;

-- Performance: 
--   - Without index: O(n) full table scan
--   - With B-tree: O(log n + k) where k = results
--   - Expected speedup: 2-3x at 100K rows


-- ============================================================================
-- Example 2: Open-Ended Range (Good for B-tree)
-- ============================================================================

-- Find all products above threshold
SELECT id, name, price FROM products
WHERE price >= 75.0;

-- Performance:
--   - B-tree excellent: O(log n) to find start, then sequential read
--   - Speedup: 1.5-2x depending on selectivity


-- ============================================================================
-- Example 3: Complex Multi-Column Range (Great for Composite B-tree)
-- ============================================================================

-- Create composite B-tree for multiple columns
CREATE INDEX idx_product_region_price ON products(region, price);

-- Find mid-range products in specific regions
SELECT id, name, region, price FROM products
WHERE region IN ('North', 'South') 
  AND price BETWEEN 50.0 AND 150.0
  AND stock > 10;

-- Performance with composite index:
--   - First filter by region (uses index prefix)
--   - Then filter by price range (uses index)
--   - Then filter stock (full scan, but on smaller subset)
--   - Overall: 3-5x faster than full table scan


-- ============================================================================
-- Example 4: Selectivity Analysis
-- ============================================================================

-- GOOD: Low selectivity (2% of table) - B-tree shines!
SELECT * FROM products WHERE price BETWEEN 1000.0 AND 1005.0;
-- Expected: 10x speedup with B-tree

-- OKAY: Medium selectivity (20% of table) - B-tree helps
SELECT * FROM products WHERE price BETWEEN 100.0 AND 200.0;
-- Expected: 1.5x speedup with B-tree

-- POOR: High selectivity (80% of table) - Full scan may be better
SELECT * FROM products WHERE price BETWEEN 10.0 AND 190.0;
-- Expected: Maybe slower with B-tree overhead


-- ============================================================================
-- Example 5: Query Optimization Patterns
-- ============================================================================

-- PATTERN 1: BETWEEN (Optimal for B-tree)
SELECT * FROM orders WHERE order_date BETWEEN '2024-01-01' AND '2024-12-31';

-- PATTERN 2: Comparison operators (Optimal for B-tree)
SELECT * FROM users WHERE age > 18 AND age < 65;

-- PATTERN 3: Inverted range (Still good for B-tree)
SELECT * FROM sales WHERE amount < 10.0 OR amount > 1000.0;

-- PATTERN 4: Prefix matching (Good for composite B-tree)
SELECT * FROM products 
WHERE category = 'Electronics'  -- Exact match
  AND price > 500.0;            -- Range match


-- ============================================================================
-- Example 6: Anti-Patterns (Not ideal for B-tree)
-- ============================================================================

-- ANTI-PATTERN 1: Full table scan (high selectivity)
-- Avoids: SELECT * FROM large_table WHERE status = 'active';
-- Use: Hash index instead for equality

-- ANTI-PATTERN 2: Functions on indexed column
-- Avoids: SELECT * FROM users WHERE UPPER(name) LIKE 'J%';
-- Better: SELECT * FROM users WHERE name LIKE 'J%';

-- ANTI-PATTERN 3: Complex expressions
-- Avoids: SELECT * FROM orders WHERE amount * quantity > 1000;
-- Better: Compute in application or add materialized column


-- ============================================================================
-- Example 7: Performance Tuning for Range Queries
-- ============================================================================

-- Strategy 1: Filter order matters
-- This query with index on (country, city, price):
SELECT * FROM locations
WHERE country = 'USA'           -- Exact: narrows significantly
  AND city LIKE 'New%'          -- Range: further narrows
  AND price > 50;               -- Range: use index for price range

-- Strategy 2: Use BETWEEN for clarity and optimization
-- This is automatically optimized better:
SELECT * FROM products
WHERE price BETWEEN 50.0 AND 150.0;

-- Strategy 3: Pre-aggregate for repeated queries
CREATE VIEW premium_products AS
SELECT * FROM products
WHERE price > 200.0;

-- Then query is much faster:
SELECT * FROM premium_products WHERE category = 'Electronics';


-- ============================================================================
-- Example 8: Batch Operations with Range Indexes
-- ============================================================================

-- Create monthly indexes for time-series data
CREATE INDEX idx_orders_date_202401 ON orders(order_date) 
WHERE YEAR(order_date) = 2024 AND MONTH(order_date) = 1;

-- Efficient queries on specific time ranges
SELECT SUM(amount) FROM orders
WHERE order_date BETWEEN '2024-01-01' AND '2024-01-31';

-- Index helps: O(log n + k) vs O(n)


-- ============================================================================
-- Example 9: Cost Analysis Guide
-- ============================================================================

/*
Rule of Thumb: When to Create B-tree Index

Create B-tree index if:
✓ Table has > 10,000 rows
✓ Predicate selectivity < 50% (returns <50% of rows)
✓ Query will run multiple times (amortize index cost)
✓ Range queries are frequent on this column

Don't create B-tree index if:
✗ Table has < 1,000 rows (overhead > benefit)
✗ Predicate selectivity > 80% (almost all rows anyway)
✗ Only equality queries (use hash index instead)
✗ Memory is critically constrained

Cost calculation:
  Index size: ~100 bytes per row
  Build time: ~100 microseconds per row (one-time)
  Query benefit: 2-3x faster for selective queries
  Break-even queries: 5-10 range queries

Example:
  100K products table
  Index size: ~10 MB
  Build time: ~10 seconds
  Break-even: 5-10 range queries
  Benefit: Paid back within minutes of usage
*/


-- ============================================================================
-- Example 10: Real-World Optimization Scenario
-- ============================================================================

-- Original query: SLOW (full table scan)
SELECT order_id, customer_name, amount, order_date
FROM orders
WHERE order_date >= '2024-01-01'
  AND order_date < '2024-02-01'
  AND amount > 100.0
ORDER BY order_date;

-- Analysis:
--   Table size: 5 million orders
--   Selectivity: ~10% (400K orders in range)
--   Current performance: 5-10 seconds (scan all 5M rows)

-- Optimization 1: Create range index on order_date
CREATE INDEX idx_orders_date ON orders(order_date);

-- Performance after index:
--   Improved: ~500ms (2-10x faster)
--   Uses B-tree to find date range quickly

-- Optimization 2: Add composite index for better filtering
CREATE INDEX idx_orders_date_amount ON orders(order_date, amount);

-- Performance with composite index:
--   Further improved: ~100ms (5-50x faster)
--   Uses B-tree to filter both date and amount efficiently

-- Final optimized query plan:
-- 1. Use idx_orders_date_amount to find matching order_date range
-- 2. Apply amount > 100 filter using same index
-- 3. Return result set (~400K orders)
-- Total time: ~100ms (vs original 10 seconds)
-- Improvement: 100x faster!
