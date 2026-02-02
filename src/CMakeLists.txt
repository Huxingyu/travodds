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
include(CheckCXXSourceCompiles)
message(STATUS "PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")

# 添加包含目录
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/thirdparty
    ${PROJECT_SOURCE_DIR}/thirdparty/asio-1.13.0/include
    ${PROJECT_SOURCE_DIR}/thirdparty/tao
)

# 设置源文件
if(WIN32)
file(GLOB_RECURSE ALL_HEADERS
    ${PROJECT_SOURCE_DIR}/include/*.h
    ${PROJECT_SOURCE_DIR}/include/**/*.h
    ${PROJECT_SOURCE_DIR}/src/*.h
    ${PROJECT_SOURCE_DIR}/src/**/*.h
    )
endif(WIN32)

file(GLOB_RECURSE ALL_SOURCES
    ${PROJECT_SOURCE_DIR}/src/*.cpp
    ${PROJECT_SOURCE_DIR}/src/**/*.cpp
)

set(${PROJECT_NAME}_source_files
    ${ALL_HEADERS}
    ${ALL_SOURCES}
    )


# if(WIN32 AND (MSVC OR MSVC_IDE))
#     list(APPEND ${PROJECT_NAME}_source_files
#         ${PROJECT_SOURCE_DIR}/src/TRAVODDS.rc
#         )
# endif()
if(BUILD_SHARED_LIBS)
    set(LIB_TARGET_NAME ${PROJECT_NAME})
else()
    set(LIB_TARGET_NAME ${PROJECT_NAME}static)
endif()

#创建库
add_library(${LIB_TARGET_NAME} ${${PROJECT_NAME}_source_files})

set_target_properties(${LIB_TARGET_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR} 
    )

#添加安全宏
if(ENABLE_DDS_SECURITY)
    message(STATUS "DDS Security enabled - DDSSECURITY macro will be defined")
    target_compile_definitions(${LIB_TARGET_NAME}
    PRIVATE
    DDSSECURITY
    )
else()
    message(STATUS "DDS Security disabled")
endif()

option(INTERNAL_DEBUG "Activate developer debug messages" OFF)
if(BUILD_SHARED_LIBS)
target_compile_definitions(${LIB_TARGET_NAME}
    PRIVATE
    _DDS_DLL_FILE
    )
else()
target_compile_definitions(${LIB_TARGET_NAME}
    PRIVATE
    _DDS_STATIC_LIB
    )
endif()
if(WIN32)
target_compile_definitions(${LIB_TARGET_NAME}
    PRIVATE
    _WINDOWS
    _USRDLL
    )
else()
target_compile_definitions(${LIB_TARGET_NAME}
    PRIVATE
    _USRDLL
    )
target_compile_options(${LIB_TARGET_NAME} PRIVATE -fPIC)
target_link_libraries(${LIB_TARGET_NAME} 
    PRIVATE
    pthread 
    rt
    dl
    )
endif()

if(MSVC OR MSVC_IDE)
    add_definitions(-DUNICODE -D_UNICODE)

    set_target_properties(${LIB_TARGET_NAME} PROPERTIES
        RELEASE_POSTFIX -${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        RELWITHDEBINFO_POSTFIX -${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        DEBUG_POSTFIX d-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        )

    get_target_property(TARGET_TYPE ${LIB_TARGET_NAME} TYPE)
    if(TARGET_TYPE STREQUAL "SHARED_LIBRARY")
        # Export symbols in DLL library
        set_target_properties(${LIB_TARGET_NAME} PROPERTIES
            PDB_NAME_DEBUG ${LIB_TARGET_NAME}d-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
            PDB_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}/symbols"
            )
    else()
        # Rename the library to have a "lib" before.
        set_target_properties(${LIB_TARGET_NAME} PROPERTIES
            OUTPUT_NAME ${LIB_TARGET_NAME}
            COMPILE_PDB_NAME_DEBUG ${LIB_TARGET_NAME}d-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
            COMPILE_PDB_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}/symbols"
            )
    endif()

    if("${CMAKE_SYSTEM_NAME}" STREQUAL "WindowsStore")
        set_target_properties(${LIB_TARGET_NAME} PROPERTIES VS_WINRT_COMPONENT "true")
    endif()
else()
    set_target_properties(${LIB_TARGET_NAME} PROPERTIES
        DEBUG_POSTFIX d
        )
    if(BUILD_SHARED_LIBS)
        set(VERSION_SCRIPT_SUPPORT_FLAGS -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/libtravodds.version)
        set(OLD_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
        set(CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS} ${VERSION_SCRIPT_SUPPORT_FLAGS})
        CHECK_CXX_SOURCE_COMPILES("int main(int, char**){return 0;}" HAS_VERSION_SCRIPT_SUPPORT)
        set(CMAKE_REQUIRED_FLAGS ${OLD_CMAKE_REQUIRED_FLAGS})
        unset(OLD_CMAKE_REQUIRED_FLAGS)
        if(HAS_VERSION_SCRIPT_SUPPORT)
            message(STATUS "Using linker version script to select exported symbols")
            target_link_options(${LIB_TARGET_NAME} PRIVATE ${VERSION_SCRIPT_SUPPORT_FLAGS})
        endif()
    endif() 
endif()

###############################################################################
# 打包
###############################################################################

# 安装公共头文件
set(PUBLIC_HEADERS
    ${PROJECT_SOURCE_DIR}/include/cdr
    ${PROJECT_SOURCE_DIR}/include/common
    ${PROJECT_SOURCE_DIR}/include/dcps
    ${PROJECT_SOURCE_DIR}/include/qos
    ${PROJECT_SOURCE_DIR}/include/type
    ${PROJECT_SOURCE_DIR}/include/xtypes
    ${PROJECT_SOURCE_DIR}/include/security
    )
install(DIRECTORY ${PUBLIC_HEADERS}
    DESTINATION ${INCLUDE_INSTALL_DIR}
    COMPONENT headers
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*.ipp"
    PATTERN "*impl.h" EXCLUDE # 排除 *impl.h 文件
    )

file(GLOB PUBLIC_HEADER_FILES ${PROJECT_SOURCE_DIR}/include/*.h)

install(FILES  ${PUBLIC_HEADER_FILES}
    DESTINATION ${INCLUDE_INSTALL_DIR}
    COMPONENT headers
    )


# 安装软件包
install(TARGETS ${LIB_TARGET_NAME}
    EXPORT ${PROJECT_NAME}-targets
    COMPONENT libraries
    RUNTIME DESTINATION "${BIN_INSTALL_DIR}${MSVCARCH_DIR_EXTENSION}"
    LIBRARY DESTINATION "${LIB_INSTALL_DIR}${MSVCARCH_DIR_EXTENSION}"
    ARCHIVE DESTINATION "${LIB_INSTALL_DIR}${MSVCARCH_DIR_EXTENSION}"
    INCLUDES DESTINATION ${INCLUDE_INSTALL_DIR}
)

# 安装 CMake 配置文件
get_target_property(TARGET_TYPE ${LIB_TARGET_NAME} TYPE)
if(TARGET_TYPE STREQUAL "SHARED_LIBRARY")
    set(TROVADDS_LINKING shared)
else()
    set(TROVADDS_LINKING static)
endif()

if(INSTALLER_PLATFORM)
    set(INSTALL_DESTINATION_PATH ${DATA_INSTALL_DIR}/${PROJECT_NAME}-${INSTALLER_PLATFORM}/cmake)
else()
    set(INSTALL_DESTINATION_PATH ${DATA_INSTALL_DIR}/${PROJECT_NAME}/cmake${MSVCARCH_DIR_EXTENSION_EXT})
endif()




install(EXPORT ${PROJECT_NAME}-targets
    DESTINATION ${INSTALL_DESTINATION_PATH}
    FILE ${PROJECT_NAME}-${TROVADDS_LINKING}-targets.cmake
    NAMESPACE TRAVODDS::
    COMPONENT cmake
    )

# VS查找和安装调试符号文件（PDB 文件）
if(MSVC OR MSVC_IDE)

    # first try dll symbols
    get_target_property(PDB_FILE ${LIB_TARGET_NAME} PDB_NAME_DEBUG)
    if(PDB_FILE)
        get_target_property(PDB_DIR ${LIB_TARGET_NAME} PDB_OUTPUT_DIRECTORY_DEBUG)
        set(PDB_FILE "${PDB_DIR}/${PDB_FILE}.pdb")
    else()
        # fallback to static lib symbols
        get_target_property(PDB_FILE ${LIB_TARGET_NAME} COMPILE_PDB_NAME_DEBUG)
        if(PDB_FILE)
            get_target_property(PDB_DIR ${LIB_TARGET_NAME} COMPILE_PDB_OUTPUT_DIRECTORY_DEBUG)
            set(PDB_FILE "${PDB_DIR}/${PDB_FILE}.pdb")
        endif()
    endif()

    # install symbols if any
    if(PDB_FILE)
        install(FILES ${PDB_FILE}
            DESTINATION ${LIB_INSTALL_DIR}${MSVCARCH_DIR_EXTENSION}
            COMPONENT symbols
            CONFIGURATIONS Debug
            )
    endif()
endif()

###############################################################################
# 创建 CMake package config 文件
###############################################################################

# 生成 config.cmake 文件
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_file(
    "${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
    @ONLY
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake"
    DESTINATION ${INSTALL_DESTINATION_PATH}
    COMPONENT cmake
)
