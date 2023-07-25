project(OrderBook_Test)

find_package(pybind11 REQUIRED)
find_package(GTest CONFIG REQUIRED)

add_executable(${PROJECT_NAME} test.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE ../include)

target_link_libraries(${PROJECT_NAME} PRIVATE
  gtest_main
  gtest
  pybind11::embed
  pthread)

target_compile_options(${PROJECT_NAME} PRIVATE -O3 -g)
