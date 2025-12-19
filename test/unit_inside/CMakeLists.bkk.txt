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
# 设置 C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# 引入 FetchContent 模块（必须
include(FetchContent)
# 声明：定义 googletest 的来源（本地路径）
FetchContent_Declare(
  googletest
  URL "${CMAKE_CURRENT_SOURCE_DIR}/../googletest-1.17.0.zip"
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

file(GLOB  HEADERS_CPP 
	"*.h")
file(GLOB  SOURCES_CPP "*.cpp")

# 添加DDS源码 集成编译
file(GLOB_RECURSE DDS_CPP 
"${CMAKE_CURRENT_SOURCE_DIR}/../../include/*.h"
"${CMAKE_CURRENT_SOURCE_DIR}/../../src/*.h"
"${CMAKE_CURRENT_SOURCE_DIR}/../../src/*.cpp"
)

# VS项目中DDS源码单独分类
source_group("DDS" FILES ${DDS_CPP})

add_executable(${PROJECT_NAME} ${SOURCES_CPP} ${HEADERS_CPP} ${DDS_CPP})

# 添加头文件路径
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty
    ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/asio-1.13.0/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/tao
)

if (MSVC)
# windows下设置静态库宏 使导出宏不生效
target_compile_definitions(${PROJECT_NAME} PRIVATE _DDS_STATIC_LIB)
# windows下添加 /utf-8 处理因编码格式导致的编译问题
target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()

target_link_libraries(${PROJECT_NAME}
  GTest::gtest_main   # GTest 核心库
)


include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})
