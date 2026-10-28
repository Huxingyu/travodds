cmake_minimum_required(VERSION 3.16)
project(datasharing_benchmark LANGUAGES CXX)
find_package(TRAVODDS 1 REQUIRED)
find_package(Threads REQUIRED)
add_executable(datasharing_benchmark benchmark.cpp
    ../../../example/cpp/data_sharing/demo.cpp
    ../../../example/cpp/data_sharing/sample.cpp)
target_compile_features(datasharing_benchmark PRIVATE cxx_std_17)
target_include_directories(datasharing_benchmark PRIVATE
    ../../../example/cpp/data_sharing ${TRAVODDS_INCLUDE_DIRS})
target_link_libraries(datasharing_benchmark PRIVATE TRAVODDS::TRAVODDS Threads::Threads)
