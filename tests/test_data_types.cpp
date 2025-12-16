#include <gtest/gtest.h>
#include "lyradb/data_types.h"

namespace lyradb {
namespace test {

TEST(DataTypesTest, TypeToString) {
    EXPECT_EQ(Type::to_string(DataType::INT32), "int32");
    EXPECT_EQ(Type::to_string(DataType::FLOAT64), "float64");
    EXPECT_EQ(Type::to_string(DataType::STRING), "string");
}

TEST(DataTypesTest, TypeSizeBytes) {
    EXPECT_EQ(Type::size_bytes(DataType::INT32), 4);
    EXPECT_EQ(Type::size_bytes(DataType::INT64), 8);
    EXPECT_EQ(Type::size_bytes(DataType::FLOAT32), 4);
}

TEST(DataTypesTest, NullBitmap) {
    NullBitmap bitmap(1024);
    
    bitmap.set_null(5, true);
    EXPECT_TRUE(bitmap.is_null(5));
    
    bitmap.set_null(10, false);
    EXPECT_FALSE(bitmap.is_null(10));
}

} // namespace test
} // namespace lyradb
