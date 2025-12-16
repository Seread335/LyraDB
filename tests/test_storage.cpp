#include <gtest/gtest.h>
#include "lyradb/schema.h"

namespace lyradb {
namespace test {

TEST(SchemaTest, CreateSchema) {
    std::vector<ColumnDef> cols;
    cols.push_back(ColumnDef("id", DataType::INT64));
    cols.push_back(ColumnDef("name", DataType::STRING));
    
    Schema schema(cols);
    EXPECT_EQ(schema.num_columns(), 2);
}

TEST(SchemaTest, FindColumn) {
    Schema schema;
    schema.add_column(ColumnDef("age", DataType::INT32));
    
    auto col = schema.find_column("age");
    EXPECT_NE(col, nullptr);
    EXPECT_EQ(col->name, "age");
}

} // namespace test
} // namespace lyradb
