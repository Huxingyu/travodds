cmake_minimum_required(VERSION 3.10)
project(dds-security-sm LANGUAGES CXX)

# 1. 基础设置
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. 平台差异处理
if(UNIX)
    message(STATUS "Configuring for Unix/Linux")
endif()

# 3. 引入Travodds配置
find_package(Travodds REQUIRED)

# 输出 TravoDDS 信息
message(STATUS "Found TravoDDS: ${Travodds_VERSION} (${Travodds_DIR})")

# 4. 收集源文件
file(GLOB_RECURSE SM_SECURITY_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

# 5. 定义动态库目标
add_library(dds_security_sm SHARED ${SM_SECURITY_SOURCES})

# 6. 设置头文件搜索路径（顺序：当前目录、上一级目录、travodds目录）
target_include_directories(dds_security_sm
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/travodds
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
        ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/tao
)

# 7. 查找依赖库
if(UNIX)
    find_package(OpenSSL REQUIRED)
    find_library(TINYXML2_LIBRARY NAMES tinyxml2)
    if(NOT TINYXML2_LIBRARY)
        message(FATAL_ERROR "tinyxml2 library not found")
    endif()
endif()

# 8. 链接库
if(UNIX)
    target_link_libraries(dds_security_sm 
        OpenSSL::SSL 
        OpenSSL::Crypto
    )
    
    # 链接Travodds库
    target_link_libraries(dds_security_sm TRAVODDS::travodds)
    
    # 链接tinyxml2库
    target_link_libraries(dds_security_sm ${TINYXML2_LIBRARY})
endif()

# 9. 测试
enable_testing()
find_package(GTest REQUIRED)

option(COVERAGE "COVERAGE TEST" OFF)
if(COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
    add_link_options(--coverage)
endif()

# 创建测试可执行文件
file(GLOB TEST_SOURCES test/*.cpp)
foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    
    # 链接库
    target_link_libraries(${test_name}
        dds_security_sm 
        GTest::gtest 
        GTest::gtest_main
        OpenSSL::SSL 
        OpenSSL::Crypto
    )
    
    # 链接Travodds库
    target_link_libraries(${test_name} TRAVODDS::travodds)
    
    # 链接tinyxml2库
    target_link_libraries(${test_name} ${TINYXML2_LIBRARY})
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

# 10. 安装
install(TARGETS dds_security_sm
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

