cmake_minimum_required(VERSION 3.10)
project(dds-security-sm LANGUAGES CXX)

# 1. 基础设置
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. 平台差异处理
if(WIN32)
    message(STATUS "Configuring for Windows")
    # Windows特有设置
elseif(UNIX)
    if(EXISTS "/etc/openEuler-release")
        message(STATUS "Configuring for openEuler")
        # openEuler特有设置
    elseif(EXISTS "/etc/lsb-release" OR EXISTS "/etc/os-release")
        message(STATUS "Configuring for Ubuntu")
       # Ubuntu特有设置
    endif()
endif()

# 3. 引入Travodds配置
find_package(Travodds REQUIRED)
if(NOT Travodds_FOUND)
    message(STATUS "Travodds package not found")
endif()

# 4. 路径检查函数
# 检查文件是否存在，如果不存在则尝试在上级目录查找
function(CHECK_FILE_PATH FILE_PATH OUTPUT_VAR)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${FILE_PATH}")
        set(${OUTPUT_VAR} "${CMAKE_CURRENT_SOURCE_DIR}/${FILE_PATH}" PARENT_SCOPE)
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../${FILE_PATH}")
        set(${OUTPUT_VAR} "${CMAKE_CURRENT_SOURCE_DIR}/../${FILE_PATH}" PARENT_SCOPE)
    else()
        message(WARNING "File not found: ${FILE_PATH} (checked current and parent directory)")
        set(${OUTPUT_VAR} "" PARENT_SCOPE)
    endif()
endfunction()

# 批量检查文件路径的函数，只包含存在的文件
function(CHECK_FILE_PATHS FILE_LIST OUTPUT_VAR)
    set(RESULT_LIST "")
    foreach(FILE_PATH ${FILE_LIST})
        CHECK_FILE_PATH("${FILE_PATH}" RESOLVED_PATH)
        if(RESOLVED_PATH)
            list(APPEND RESULT_LIST "${RESOLVED_PATH}")
        endif()
    endforeach()
    set(${OUTPUT_VAR} "${RESULT_LIST}" PARENT_SCOPE)
endfunction()

# 4. 源码与头文件
set(SM_SECURITY_SOURCES
    src/accesscontrol/DistinguishedName.cpp
    src/accesscontrol/Permissions.cpp
    src/accesscontrol/PermissionsParser.cpp
    src/accesscontrol/GovernanceParser.cpp
    src/cryptography/SM4GCMGMAC_KeyExchange.cpp
    src/cryptography/SM4GCMGMAC_KeyFactory.cpp
    src/cryptography/SM4GCMGMAC_Transform.cpp
    src/auth/PKIGM.cpp
    src/auth/GMHelper.cpp
    src/auth/PKcs11Provider.cpp
    src/rtps/security/common/SharedSecretHandle.cpp
    src/PropertyPolicy.cpp
    src/sm/SMAlgorithm.cpp
)

# 解析源码文件路径
CHECK_FILE_PATHS("${SM_SECURITY_SOURCES}" SM_SECURITY_SOURCES_RESOLVED)

set(SM_PUBLIC_HEADERS
    include/Security/AccessControl.h
    include/Security/SecurityException.h
    include/Security/SecurityCommon.h
    include/Security/AuthenticationListener.h
    include/Security/Authentication.h
    include/Security/CryCommonSource.h
    include/Security/PKcs11Provider.h
    include/Security/numericconstants.h
    include/accesscontrol/Permissions.h
    include/accesscontrol/AccessPermissionsHandle.h
    include/accesscontrol/DistinguishedName.h
    include/accesscontrol/EndpointSecurityAttributes.h
    include/accesscontrol/ParticipantSecurityAttributes.h
    include/accesscontrol/PermissionsParser.h
    include/accesscontrol/PermissionsTypes.h    
    include/auth/GMHandshakeHandle.h
    include/auth/GMIdentityHandle.h
    include/auth/GMHelper.h
    include/auth/PKIGM.h
    include/Security/AccessControlListener.h
    include/Security/CryptoTransform.h
    include/Security/CryptoKeyExchange.h
    include/Security/CryptoKeyFactory.h
    include/cryptography/CryptoTypes.h
    include/cryptography/SM4GCMGMAC_Types.h
    include/cryptography/SM4GCMGMAC_KeyExchange.h
    include/cryptography/SM4GCMGMAC_KeyFactory.h
    include/cryptography/SM4GCMGMAC_Transform.h     
)

# 解析公共头文件路径
CHECK_FILE_PATHS("${SM_PUBLIC_HEADERS}" SM_PUBLIC_HEADERS_RESOLVED)

set(SM_PRIVATE_HEADERS
    src/accesscontrol/DistinguishedName.h
    src/accesscontrol/Permissions.h
    src/accesscontrol/PermissionsParser.h
    src/accesscontrol/GovernanceParser.h
    src/cryptography/SM4GCMGMAC_KeyExchange.h
    src/cryptography/SM4GCMGMAC_KeyFactory.h
    src/cryptography/SM4GCMGMAC_Transform.h
    src/auth/PKIGM.h
    src/auth/GMHelper.h
    src/auth/PKcs11Provider.h
)

# 解析私有头文件路径
CHECK_FILE_PATHS("${SM_PRIVATE_HEADERS}" SM_PRIVATE_HEADERS_RESOLVED)

# 定义动态库目标
add_library(dds_security_sm SHARED
    ${SM_SECURITY_SOURCES_RESOLVED}
    ${SM_PUBLIC_HEADERS_RESOLVED}
    ${SM_PRIVATE_HEADERS_RESOLVED}
)

# 为库目标精确地指定头文件搜索路径
target_include_directories(dds_security_sm
    PUBLIC
        # PUBLIC: 编译本库和链接本库的其他目标都需要这个路径
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../include  # 上级目录的include
        ${CMAKE_CURRENT_SOURCE_DIR}/src  # 添加src路径以便测试可以访问
        ${CMAKE_CURRENT_SOURCE_DIR}/../src  # 上级目录的src
		${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/tao/
    PRIVATE
        # PRIVATE: 仅在编译本库时内部需要这个路径
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/../src  # 上级目录的src
)

# 设置公共头文件属性，以便正确安装
set_target_properties(dds_security_sm PROPERTIES
    PUBLIC_HEADER "${SM_PUBLIC_HEADERS_RESOLVED}"
)

# 5. 平台/中间件特有链接
if(WIN32)
    # Windows下的链接库
elseif(UNIX)
    # Linux下的链接库
    find_package(OpenSSL REQUIRED)
    # 查找tinyxml2库
    find_package(tinyxml2 QUIET)
    if(NOT tinyxml2_FOUND)
        # 如果找不到tinyxml2包，尝试使用pkg-config
        find_library(TINYXML2_LIBRARY NAMES tinyxml2)
        if(TINYXML2_LIBRARY)
            message(STATUS "Found tinyxml2 library: ${TINYXML2_LIBRARY}")
        else()
            message(WARNING "tinyxml2 library not found, some features may not work")
        endif()
    endif()
    
    # 链接库
    target_link_libraries(dds_security_sm 
        OpenSSL::SSL 
        OpenSSL::Crypto
    )
    
    # 链接Travodds库
    if(TARGET TRAVODDS::travodds)
        target_link_libraries(dds_security_sm TRAVODDS::travodds)
    elseif(TARGET TRAVODDS::TRAVODDS)
        target_link_libraries(dds_security_sm TRAVODDS::TRAVODDS)
    else()
        message(WARNING "Travodds target not found, trying to link manually")
        find_library(TRAVODDS_LIBRARY NAMES travodds PATHS /usr/local/lib)
        if(TRAVODDS_LIBRARY)
            target_link_libraries(dds_security_sm ${TRAVODDS_LIBRARY})
        endif()
    endif()
    
    # 链接tinyxml2库
    if(TARGET tinyxml2::tinyxml2)
        target_link_libraries(dds_security_sm tinyxml2::tinyxml2)
    elseif(TINYXML2_LIBRARY)
        target_link_libraries(dds_security_sm ${TINYXML2_LIBRARY})
    endif()
endif()

# 6. 测试
enable_testing()
# 查找Google Test
find_package(GTest REQUIRED)
if(GTest_FOUND)
    message(STATUS "Google Test found: ${GTEST_LIBRARIES}")
else()
    message(FATAL_ERROR "Google Test not found. Please install libgtest-dev")
endif()
# 覆盖率测试
OPTION(COVERAGE "COVERAGE TEST" OFF) # 默认关闭
if(COVERAGE)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
    add_link_options(--coverage)
endif()


# 创建测试可执行文件
file(GLOB TEST_SOURCES test/*.cpp)
foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    
    # 链接库列表
    set(TEST_LINK_LIBS
        dds_security_sm 
        GTest::gtest 
        GTest::gtest_main
        OpenSSL::SSL 
        OpenSSL::Crypto
    )
    
    # 链接Travodds库
    if(TARGET TRAVODDS::travodds)
        list(APPEND TEST_LINK_LIBS TRAVODDS::travodds)
    elseif(TARGET TRAVODDS::TRAVODDS)
        list(APPEND TEST_LINK_LIBS TRAVODDS::TRAVODDS)
    else()
        find_library(TRAVODDS_LIBRARY NAMES travodds PATHS /usr/local/lib)
        if(TRAVODDS_LIBRARY)
            list(APPEND TEST_LINK_LIBS ${TRAVODDS_LIBRARY})
        endif()
    endif()
    
    # 链接tinyxml2库
    if(TARGET tinyxml2::tinyxml2)
        list(APPEND TEST_LINK_LIBS tinyxml2::tinyxml2)
    elseif(TINYXML2_LIBRARY)
        list(APPEND TEST_LINK_LIBS ${TINYXML2_LIBRARY})
    else()
        find_library(TINYXML2_LIBRARY NAMES tinyxml2)
        if(TINYXML2_LIBRARY)
            list(APPEND TEST_LINK_LIBS ${TINYXML2_LIBRARY})
        endif()
    endif()
    
    target_link_libraries(${test_name} ${TEST_LINK_LIBS})
    add_test(NAME ${test_name} COMMAND ${test_name})
    # gtest_discover_tests(${test_name})
endforeach()

# 7. 安装
install(TARGETS dds_security_sm
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
install(DIRECTORY include/dds_security_sm DESTINATION include)

