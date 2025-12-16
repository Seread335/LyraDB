# LyraDB Formats Library CMakeLists.txt
# This file builds the LyraDB formats library for .lyradb, .lyradbite, and .lyra files

cmake_minimum_required(VERSION 3.12)

project(lyradb_formats VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")

# Add the formats library
add_library(lyradb_formats STATIC
    src/formats/lyradb_formats.cpp
)

# Include directories
target_include_directories(lyradb_formats PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Set library properties
set_target_properties(lyradb_formats PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    OUTPUT_NAME lyradb_formats
)

# For Windows
if(WIN32)
    target_compile_definitions(lyradb_formats PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Create an example executable to test the library
add_executable(test_formats
    examples/test_formats.cpp
)

target_include_directories(test_formats PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(test_formats PRIVATE
    lyradb_formats
)

# Installation
install(TARGETS lyradb_formats
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/lyradb
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# Export targets
export(TARGETS lyradb_formats
    FILE "${CMAKE_BINARY_DIR}/LyraDBFormatsTargets.cmake"
)

message(STATUS "LyraDB Formats Library Configuration Complete")
message(STATUS "  - Library: lyradb_formats")
message(STATUS "  - Version: ${PROJECT_VERSION}")
message(STATUS "  - C++ Standard: 17")
