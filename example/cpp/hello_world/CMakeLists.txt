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
project(hello_world LANGUAGES CXX)
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

message(STATUS "Configuring hello_world example...")
file(GLOB HELLO_WORLD_HEADERS_CPP "*.h")
file(GLOB HELLO_WORLD_SOURCES_CPP "*.cpp")

add_executable(${PROJECT_NAME} ${HELLO_WORLD_SOURCES_CPP} ${HELLO_WORLD_HEADERS_CPP})
# target_compile_definitions(${PROJECT_NAME}
#     PRIVATE
#     _DDS_STATIC_LIB
#     )
target_include_directories(${PROJECT_NAME} PRIVATE ${TRAVODDS_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PRIVATE /mnt/d/travodds/travodds/include)

#target_link_libraries(${PROJECT_NAME} PRIVATE TRAVODDS::TRAVODDS_STATIC)
target_link_libraries(${PROJECT_NAME} PRIVATE TRAVODDS::TRAVODDS)
install(TARGETS ${PROJECT_NAME}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)
