cmake_minimum_required(VERSION 3.16)
project(payloadpool_test LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(TRAVODDS 1 REQUIRED)

include(FetchContent)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
FetchContent_Declare(googletest
    URL "${CMAKE_CURRENT_SOURCE_DIR}/../googletest-1.17.0.zip")
FetchContent_MakeAvailable(googletest)

add_executable(payloadpool_test test_payloadpool.cpp test_topicpayloadpool.cpp test_loan.cpp
    test_threadpool_progress.cpp
    test_native_key_admission.cpp
    test_native_initialization.cpp
    test_reader_payloadpool.cpp
    test_datasharingqos.cpp
    test_datasharingdiscovery.cpp
    test_receiversocket.cpp
    test_logger.cpp
    test_payloadintegration.cpp
    test_writerproxy.cpp
    ../../example/cpp/hello_world/HelloWorld.cpp
    ../../example/cpp/hello_world/HelloWorldTypeSupport.cpp)
target_include_directories(payloadpool_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../src"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../example/cpp/hello_world"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/tao"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/asio-1.13.0/include"
    ${TRAVODDS_INCLUDE_DIRS})
target_link_libraries(payloadpool_test PRIVATE TRAVODDS::TRAVODDS GTest::gtest_main)
enable_testing()
include(GoogleTest)
gtest_discover_tests(payloadpool_test)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    foreach(progress_target native_initialization reader_payloadpool)
        add_executable(${progress_target}_test test_${progress_target}.cpp)
        target_include_directories(${progress_target}_test PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/../../src"
            "${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty"
            "${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/tao"
            "${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/asio-1.13.0/include"
            ${TRAVODDS_INCLUDE_DIRS})
        target_link_libraries(${progress_target}_test PRIVATE TRAVODDS::TRAVODDS GTest::gtest_main)
        gtest_discover_tests(${progress_target}_test)
    endforeach()

    add_executable(native_key_admission_test test_native_key_admission.cpp test_threadpool_progress.cpp)
    target_include_directories(native_key_admission_test PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/../../src"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/tao"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/asio-1.13.0/include"
        ${TRAVODDS_INCLUDE_DIRS})
    target_link_libraries(native_key_admission_test PRIVATE TRAVODDS::TRAVODDS GTest::gtest_main)
    gtest_discover_tests(native_key_admission_test)

    add_executable(sharedpayloadpool_test test_sharedpayloadpool.cpp)
    target_include_directories(sharedpayloadpool_test PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/../../src"
        ${TRAVODDS_INCLUDE_DIRS})
    target_link_libraries(sharedpayloadpool_test PRIVATE TRAVODDS::TRAVODDS GTest::gtest)
    gtest_discover_tests(sharedpayloadpool_test)
endif()
