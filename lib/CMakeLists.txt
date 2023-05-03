set(ProjectId ts_containers_lib)
project(${ProjectId} C CXX)

option(LOG_ENABLED "Enable logging" OFF)
if(LOG_ENABLED)
    add_compile_definitions(LOG)
endif()

file(GLOB_RECURSE ALL_SOURCE_FILES *.cpp *.h *.hpp)

add_library(${ProjectId} STATIC ${ALL_SOURCE_FILES})
target_include_directories(${ProjectId} PUBLIC ../)
