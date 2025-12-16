cmake_minimum_required(VERSION 3.15)
project(my_lyradb_app)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find LyraDB Formats package via Conan
find_package(lyradb_formats REQUIRED CONFIG)

# Create your application
add_executable(my_app main.cpp)

# Link against lyradb_formats
target_link_libraries(my_app PRIVATE lyradb_formats::lyradb_formats)

# Include the source files for compilation if needed
target_sources(my_app PRIVATE 
    ${CONAN_LYRADB_FORMATS_ROOT}/src/lyradb_formats.cpp
)
