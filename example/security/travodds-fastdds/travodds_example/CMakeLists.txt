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

cmake_minimum_required(VERSION 3.10)
project(ConnenctPoint LANGUAGES CXX)
if(NOT TRAVODDS_FOUND)
    find_package(TRAVODDS 1 REQUIRED)
endif()

#检测 C++11
include(CheckCXXCompilerFlag)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    check_cxx_compiler_flag(-std=c++11 SUPPORTS_CXX11)
    if(NOT SUPPORTS_CXX11)
        message(FATAL_ERROR "Compiler doesn't support C++11")
    endif()
endif()

# 设置CMAKE_BUILD_TYPE 默认为Release.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

#############################基础配置#############################
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/source-charset:utf-8>")

###############################################################################
# 警告等级
###############################################################################
if(MSVC OR MSVC_IDE)
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    endif()

    # 为 MSVC 编译器添加额外的警告选项，目的是使 MSVC 编译器的行为更加接近 GCC 或 Clang 编译器。
    # C4101 未引用的局部变量
    # C4189 已初始化但未引用的局部变量
    # C4555 表达式无效，应该是有副作用的表达式
    # C4715 函数的所有控制路径未返回值
    # C5038 数据成员初始化顺序警告
    # C4100 未引用的形式参数
    #add_compile_options(/w34101 /w34189 /w34555 /w34715 /w35038 /w44100)
	#add_compile_options(/w34189 /w34555 /w34715 /w35038)
else()
    # 添加 GCC 和 Clang 编译器配置通用的警告和链接器选项
    #set(CMAKE_CXX_FLAGS
    #    "${CMAKE_CXX_FLAGS} -Wall -Wpedantic -Wextra -Wno-unknown-pragmas -Wno-error=deprecated-declarations -Wno-switch-bool")
    #add_compile_options(-Wno-narrowing)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 15,0)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-builtins")
        endif()
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl")
    endif()
endif()

message(STATUS "Configuring example...")
#收集源文件
file(GLOB_RECURSE SOURCES_FILES
	"src/ConnectPoint.cpp"
	"src/HelloWorld.cpp"
	"src/HelloWorldTypeSupport.cpp"
)
add_executable(ConnectPoint ${SOURCES_FILES})
# target_compile_definitions(${PROJECT_NAME}
#     PRIVATE
#     _DDS_STATIC_LIB
#     )

target_link_libraries(ConnectPoint PRIVATE TRAVODDS::TRAVODDS)

if(TRAVODDS_INCLUDE_DIR)
    include_directories(${TRAVODDS_INCLUDE_DIR})
endif()
