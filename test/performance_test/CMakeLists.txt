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
project(performance_test LANGUAGES CXX)
if(NOT TRAVODDS_FOUND)
    find_package(TRAVODDS)
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

message(STATUS "Configuring performance_test example...")

# 添加包含目录
include_directories(
    ${PROJECT_SOURCE_DIR}
)

# 设置源文件
if(WIN32)
file(GLOB_RECURSE ALL_HEADERS
    ${PROJECT_SOURCE_DIR}/*.h
    )
endif(WIN32)

file(GLOB_RECURSE ALL_SOURCES
    ${PROJECT_SOURCE_DIR}/type.cpp  
    ${PROJECT_SOURCE_DIR}/typetypesupport.cpp    
    ${PROJECT_SOURCE_DIR}/entity.cpp
    ${PROJECT_SOURCE_DIR}/throughputtest.cpp
    ${PROJECT_SOURCE_DIR}/delaytest.cpp
    ${PROJECT_SOURCE_DIR}/main.cpp
)

set(${PROJECT_NAME}_source_files
    ${ALL_HEADERS}
    ${ALL_SOURCES}
    )

add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_source_files})
target_include_directories(${PROJECT_NAME} PRIVATE ${TRAVODDS_INCLUDE_DIRS})
if(WIN32)
target_link_libraries(${PROJECT_NAME} PRIVATE TRAVODDS::TRAVODDS)
else()
target_link_libraries(${PROJECT_NAME} PRIVATE TRAVODDS::TRAVODDS pthread)
endif()
install(TARGETS ${PROJECT_NAME}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)
