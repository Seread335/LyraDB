#include <benchmark/benchmark.h>
#include <lyradb.h>
#include <random>
#include <chrono>
#include <vector>
#include <string>

using namespace lyradb;

class QueryBenchmarkFixture : public benchmark::Fixture {
public:
    std::shared_ptr<Database> db;
    std::shared_ptr<Table> employees;
    std::shared_ptr<Table> departments;
    
    void SetUp(const benchmark::State&) override {
        db = std::make_shared<Database>(":memory:");
        
        // Create employees table
        Schema emp_schema;
        emp_schema.add_column(Column("id", DataType::INT32));
        emp_schema.add_column(Column("name", DataType::STRING));
        emp_schema.add_column(Column("salary", DataType::FLOAT32));
        emp_schema.add_column(Column("dept_id", DataType::INT32));
        emp_schema.add_column(Column("age", DataType::INT32));
        emp_schema.add_column(Column("active", DataType::BOOLEAN));
        
        employees = db->create_table("employees", emp_schema);
        
        // Create departments table
        Schema dept_schema;
        dept_schema.add_column(Column("id", DataType::INT32));
        dept_schema.add_column(Column("name", DataType::STRING));
        dept_schema.add_column(Column("budget", DataType::FLOAT32));
        
        departments = db->create_table("departments", dept_schema);
        
        // Insert test data
        populate_employees(10000);
        populate_departments(50);
    }
    
    void TearDown(const benchmark::State&) override {
        employees->clear();
        departments->clear();
        db.reset();
    }
    
private:
    void populate_employees(int count) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dept_dist(1, 50);
        std::uniform_real_distribution<> salary_dist(30000, 150000);
        std::uniform_int_distribution<> age_dist(22, 65);
        std::uniform_int_distribution<> active_dist(0, 1);
        
        for (int i = 1; i <= count; ++i) {
            std::vector<std::string> row;
            row.push_back(std::to_string(i));  // id
            row.push_back("Employee_" + std::to_string(i));  // name
            row.push_back(std::to_string(salary_dist(gen)));  // salary
            row.push_back(std::to_string(dept_dist(gen)));  // dept_id
            row.push_back(std::to_string(age_dist(gen)));  // age
            row.push_back(active_dist(gen) ? "1" : "0");  // active
            
            employees->insert(row);
        }
    }
    
    void populate_departments(int count) {
        for (int i = 1; i <= count; ++i) {
            std::vector<std::string> row;
            row.push_back(std::to_string(i));  // id
            row.push_back("Department_" + std::to_string(i));  // name
            row.push_back(std::to_string(500000 + i * 10000));  // budget
            
            departments->insert(row);
        }
    }
};

// Full table scan benchmark
BENCHMARK_F(QueryBenchmarkFixture, FullTableScan)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = db->execute("SELECT * FROM employees");
        benchmark::DoNotOptimize(result);
    }
}

// Simple WHERE clause benchmark
BENCHMARK_F(QueryBenchmarkFixture, SimpleWhere)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = db->execute("SELECT * FROM employees WHERE salary > 50000");
        benchmark::DoNotOptimize(result);
    }
}

// Complex WHERE with multiple conditions
BENCHMARK_F(QueryBenchmarkFixture, ComplexWhere)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = db->execute(
            "SELECT * FROM employees WHERE salary > 50000 AND age < 40 AND active = 1"
        );
        benchmark::DoNotOptimize(result);
    }
}

// GROUP BY benchmark
BENCHMARK_F(QueryBenchmarkFixture, GroupByDepartment)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = db->execute(
            "SELECT dept_id, COUNT(*) as cnt, AVG(salary) as avg_sal FROM employees GROUP BY dept_id"
        );
        benchmark::DoNotOptimize(result);
    }
}

// ORDER BY benchmark
BENCHMARK_F(QueryBenchmarkFixture, OrderBySalary)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = db->execute("SELECT * FROM employees ORDER BY salary DESC LIMIT 100");
        benchmark::DoNotOptimize(result);
    }
}

// JOIN benchmark
BENCHMARK_F(QueryBenchmarkFixture, InnerJoin)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = db->execute(
            "SELECT e.name, e.salary, d.name FROM employees e INNER JOIN departments d ON e.dept_id = d.id"
        );
        benchmark::DoNotOptimize(result);
    }
}

// Multiple aggregates
BENCHMARK_F(QueryBenchmarkFixture, MultipleAggregates)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = db->execute(
            "SELECT dept_id, COUNT(*) as cnt, SUM(salary) as total, AVG(salary) as avg, MAX(salary) as max, MIN(salary) as min FROM employees GROUP BY dept_id"
        );
        benchmark::DoNotOptimize(result);
    }
}

// Pagination benchmark
BENCHMARK_F(QueryBenchmarkFixture, PaginationLimit100)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = db->execute("SELECT * FROM employees LIMIT 100 OFFSET 0");
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_F(QueryBenchmarkFixture, PaginationLimit1000)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = db->execute("SELECT * FROM employees LIMIT 1000 OFFSET 0");
        benchmark::DoNotOptimize(result);
    }
}

// String filtering
BENCHMARK_F(QueryBenchmarkFixture, StringFilter)(benchmark::State& state) {
    for (auto _ : state) {
        // Simulating LIKE pattern matching (when implemented)
        auto result = db->execute("SELECT * FROM employees WHERE name = 'Employee_1234'");
        benchmark::DoNotOptimize(result);
    }
}

// Single row access
BENCHMARK_F(QueryBenchmarkFixture, SingleRowAccess)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = db->execute("SELECT * FROM employees WHERE id = 5000");
        benchmark::DoNotOptimize(result);
    }
}

// Large result set
BENCHMARK_F(QueryBenchmarkFixture, LargeResultSet)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = db->execute("SELECT * FROM employees WHERE salary > 0");
        benchmark::DoNotOptimize(result);
    }
}

// Complex GROUP BY with HAVING
BENCHMARK_F(QueryBenchmarkFixture, GroupByWithHaving)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = db->execute(
            "SELECT dept_id, COUNT(*) as cnt FROM employees GROUP BY dept_id HAVING COUNT(*) > 100"
        );
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_MAIN(argc, argv);
