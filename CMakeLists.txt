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

###############################################################################
# CMake 构建规则
###############################################################################
cmake_minimum_required(VERSION 3.10)

# CMAKE_BUILD_TYPE 默认为 Release
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

###############################################################################
# 项 目
###############################################################################
project(travodds VERSION "1.1.0" LANGUAGES C CXX)
set(PROJECT_NAME_LARGE "TRAVODDS")

string(TOUPPER "${PROJECT_NAME}" PROJECT_NAME_UPPER)
message(STATUS "Version: ${PROJECT_VERSION}")

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
    add_compile_options(/w34101 /w34189 /w34555 /w34715 /w35038 /w44100)
else()
    # 添加 GCC 和 Clang 编译器配置通用的警告和链接器选项
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -Wall -Wpedantic -Wextra -Wno-unknown-pragmas -Wno-error=deprecated-declarations -Wno-switch-bool")
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

###############################################################################
# 安装路径
###############################################################################
option(APPEND_PROJECT_NAME_TO_INSTALL
  "When ON headers are installed to a path ending with a folder called \
  ${PROJECT_NAME}. This avoids include directory search order issues when \
  overriding this package from a merged catkin, ament, or colcon workspace."
  OFF)

set(BIN_INSTALL_DIR bin/ CACHE PATH "Installation directory for binaries")
set(_include_dir "include/")
set(INCLUDE_INSTALL_DIR "${_include_dir}/${PROJECT_NAME}/" CACHE PATH "Installation directory for C++ headers")
unset(_include_dir)
set(LIB_INSTALL_DIR lib${LIB_SUFFIX}/ CACHE PATH "Installation directory for libraries")
if(APPEND_PROJECT_NAME_TO_INSTALL)
#  string(APPEND INCLUDE_INSTALL_DIR "/${PROJECT_NAME}/")
  string(APPEND LIB_INSTALL_DIR "/${PROJECT_NAME}/")
endif()
set(DATA_INSTALL_DIR share/ CACHE PATH "Installation directory for data")
if(WIN32)
    set(DOC_DIR "doc")
else()
    set(DOC_DIR "${DATA_INSTALL_DIR}/doc")
endif()
set(DOC_INSTALL_DIR ${DOC_DIR} CACHE PATH "Installation directory for documentation")
set(LICENSE_INSTALL_DIR ${DATA_INSTALL_DIR}/${PROJECT_NAME} CACHE PATH "Installation directory for licenses")


###############################################################################
# 默认构建动态库
###############################################################################
option(BUILD_SHARED_LIBS "Create shared libraries by default" ON)

###############################################################################
# 默认DDS 安全配置选项不启用
###############################################################################
option(ENABLE_DDS_SECURITY "Enable DDS Security features" ON)

###############################################################################
# 加载外部项目
###############################################################################
#find_package(tinyxml2 REQUIRED)

###############################################################################
# 编译库
###############################################################################
add_subdirectory(src)
