set(ProjectId main)
project(${ProjectId})

option(LOG_ENABLED "Enable logging" OFF)
if(LOG_ENABLED)
    add_compile_definitions(LOG)
endif()

file(GLOB ALL_SOURCE_FILES *.cpp *.h *.hpp)

add_executable(${ProjectId} ${ALL_SOURCE_FILES})
target_include_directories(${ProjectId} PUBLIC ../)

target_link_libraries(${PROJECT_NAME} argparse)
target_link_libraries(${PROJECT_NAME} pthread)
target_link_libraries(${PROJECT_NAME} benchmark::benchmark)
target_link_libraries(${PROJECT_NAME} ts_containers_lib)
