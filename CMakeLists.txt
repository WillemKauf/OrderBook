cmake_minimum_required (VERSION 3.13)
project(OrderBook)

set(CMAKE_CXX_STANDARD 23)

find_package(pybind11 REQUIRED)

add_executable(${PROJECT_NAME}_Client src/client_main.cpp)
add_executable(${PROJECT_NAME}_Server src/server_main.cpp)

target_include_directories(${PROJECT_NAME}_Client PRIVATE include)
target_link_libraries(${PROJECT_NAME}_Client PRIVATE pybind11::embed pthread)
target_compile_options(${PROJECT_NAME}_Client PRIVATE -O3 -g)

target_include_directories(${PROJECT_NAME}_Server PRIVATE include)
target_link_libraries(${PROJECT_NAME}_Server PRIVATE pybind11::embed pthread)
target_compile_options(${PROJECT_NAME}_Server PRIVATE -O3 -g)

add_subdirectory(test)
