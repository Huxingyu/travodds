#Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
#
#You can use this software according to the terms and conditions of
#the Mulan PSL v2.You may obtain a copy of Mulan PSL v2 at :
#http://license.coscl.org.cn/MulanPSL2
#
#THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF
#ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
#NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
#
#See the Mulan PSL v2 for more details.

cmake_minimum_required(VERSION 3.16)
project(unit_test LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED TRAVODDS_LIB)
    set(TRAVODDS_LIB "/usr/lib/libtravodds.so.1.1")
endif()

include(FetchContent)
FetchContent_Declare(
  googletest
  URL "${CMAKE_CURRENT_SOURCE_DIR}/../googletest-1.17.0.zip"
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

file(GLOB HEADERS_CPP "*.h")
file(GLOB SOURCES_CPP "*.cpp")

add_executable(${PROJECT_NAME} ${SOURCES_CPP} ${HEADERS_CPP})

if (MSVC)
target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../src"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/asio-1.13.0/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/tao"
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    ${TRAVODDS_LIB}
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})
