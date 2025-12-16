cmake_minimum_required(VERSION 3.15)
project(LyraDB_Examples)

# Include LyraDB
include_directories(../include)

# Simple example
add_executable(simple_example simple.c)

# Link with LyraDB library
# Depending on how you want to link (shared or static)
if(WIN32)
    target_link_libraries(simple_example PRIVATE ${CMAKE_SOURCE_DIR}/../lib/lyradb.lib)
else()
    target_link_libraries(simple_example PRIVATE ${CMAKE_SOURCE_DIR}/../lib/liblyradb.so)
endif()

# Or build examples as part of main project
# add_executable(simple_example simple.c)
# target_link_libraries(simple_example PRIVATE lyradb_c)
