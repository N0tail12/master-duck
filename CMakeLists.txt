cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

project(bus)

set(CMAKE_BUILD_TYPE Debug)

set(C_FILES main.c)
set(C_LIBS fdr)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

include_directories(${PROJECT_SOURCE_DIR}/libfdr)
link_directories(${PROJECT_SOURCE_DIR}/libfdr)


add_executable(${PROJECT_NAME} ${C_FILES})
target_link_libraries(${PROJECT_NAME} ${C_LIBS})

# add_executable(dijkstra dijkstra.c)
# target_link_libraries(dijkstra ${C_LIBS})