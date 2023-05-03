cmake_minimum_required(VERSION 3.22)

set(PROJECT_NAME ts_containers)

project(${PROJECT_NAME} C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Thread Sanitizer
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")

# fetch latest argparse
include(FetchContent)
FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
)
FetchContent_MakeAvailable(argparse)

include_directories("${CMAKE_SOURCE_DIR}/argparse/include")
include_directories("${CMAKE_SOURCE_DIR}/benchmark/include")

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/benchmark")
add_subdirectory(bin)
add_subdirectory(lib)
add_subdirectory(benches)
