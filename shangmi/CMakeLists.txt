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
    src/dcps/topic/topicdescriptionimpl.cpp
    src/dcps/topic/contentfilteredtopicimpl.cpp
    src/dcps/topic/multitopicimpl.cpp
    src/dcps/topic/topicimpl.cpp
    src/dcps/base/entityimpl.cpp
    src/dcps/base/statusmanager.cpp
    src/dcps/condition/guardconditionimpl.cpp
    src/dcps/condition/queryconditionimpl.cpp
    src/dcps/condition/guardcondition.cpp
    src/dcps/condition/statusconditionimpl.cpp
    src/dcps/condition/waitset.cpp
    src/dcps/condition/readconditionimpl.cpp
    src/dcps/condition/conditionimpl.cpp
    src/dcps/condition/waitsetimpl.cpp
    src/dcps/publish/publisherimpl.cpp
    src/dcps/publish/datawriterimpl.cpp
    src/dcps/domain/domainparticipantfactoryimpl.cpp
    src/dcps/domain/domainentityimpl.cpp
    src/dcps/domain/domainparticipantimpl.cpp
    src/dcps/subscribe/subscriberimpl.cpp
    src/dcps/subscribe/datareaderimpl.cpp
    src/sm/SMAlgorithm.cpp
    src/cdr/travoddscdr.cpp
    src/cdr/md5.cpp
    src/xml/tinyxml2.cpp
    src/xml/dds_xml_repo.cpp
    src/xml/dds_cpp_xml_interface.cpp
    src/common/memory/memorypool.cpp
    src/common/time/ddstimer.cpp
    src/common/log/logger.cpp
    src/common/sequence/sequence.cpp
    src/common/threads/threadpool.cpp
    src/rtps/messages/parameter.cpp
    src/rtps/messages/messageprocessor.cpp
    src/rtps/transport/messagetransmitterudp.cpp
    src/rtps/transport/messagetransmitter.cpp
    src/rtps/transport/asiocontext.cpp
    src/rtps/transport/locator.cpp
    src/rtps/transport/ipcontainer.cpp
    src/rtps/endpoint/readerproxy.cpp
    src/rtps/endpoint/participant.cpp
    src/rtps/endpoint/statelesswriter.cpp
    src/rtps/endpoint/reader.cpp
    src/rtps/endpoint/statefulreader.cpp
    src/rtps/endpoint/statelessreader.cpp
    src/rtps/endpoint/readerlocator.cpp
    src/rtps/endpoint/statefulwriter.cpp
    src/rtps/endpoint/writer.cpp
    src/rtps/endpoint/writerproxy.cpp
    src/rtps/historycache/historycache.cpp
    src/rtps/security/common/SharedSecretHandle.cpp
    src/rtps/discovery/spdp.cpp
    src/rtps/discovery/sedp.cpp
    src/rtps/discovery/discovery.cpp
    src/rtps/builtin/liveliness/livelinessassert.cpp
    src/rtps/builtin/liveliness/livelinessmanager.cpp
    src/auth/PKIGM.cpp
    src/auth/GMHelper.cpp
    src/auth/PKcs11Provider.cpp
    src/PropertyPolicy.cpp
    src/qos/qos.cpp
    src/qos/qospolicycheck.cpp
)

# 解析源码文件路径
CHECK_FILE_PATHS("${SM_SECURITY_SOURCES}" SM_SECURITY_SOURCES_RESOLVED)

set(SM_PUBLIC_HEADERS
    include/dcps/topic/typesupport.h
    include/dcps/topic/topic.h
    include/dcps/topic/contentfilteredtopic.h
    include/dcps/topic/multitopic.h
    include/dcps/topic/topicdescription.h
    include/dcps/base/status.h
    include/dcps/base/retcode.h
    include/dcps/base/entity.h
    include/dcps/listener/topiclistener.h
    include/dcps/listener/listener.h
    include/dcps/listener/domainparticipantlistener.h
    include/dcps/listener/publisherlistener.h
    include/dcps/listener/datawriterlistener.h
    include/dcps/listener/subscriberlistener.h
    include/dcps/listener/datareaderlistener.h
    include/dcps/condition/guardcondition.h
    include/dcps/condition/condition.h
    include/dcps/condition/readcondition.h
    include/dcps/condition/statuscondition.h
    include/dcps/condition/querycondition.h
    include/dcps/condition/waitset.h
    include/dcps/publish/datawriter.h
    include/dcps/publish/publisher.h
    include/dcps/domain/domainparticipantfactory.h
    include/dcps/domain/domainentity.h
    include/dcps/domain/domainparticipant.h
    include/dcps/subscribe/subscriber.h
    include/dcps/subscribe/states.h
    include/dcps/subscribe/datareader.h
    include/cdr/travoddscdr.h
    include/type/builtintopicdata.h
    include/type/basictype.h
    include/common/sequence/sequencebase.h
    include/common/sequence/sequence.h
    include/Security/AccessControl.h
    include/Security/SecurityException.h
    include/Security/SecurityCommon.h
    include/Security/AuthenticationListener.h
    include/Security/Authentication.h
    include/Security/CryCommonSource.h
    include/Security/PKcs11Provider.h
    include/travodds_dll.h
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
    include/qos/qos.h
    include/TypeSupport.h
    include/rtps/common/Token.h
    include/rtps/security/common/Handle.h
    include/rtps/security/common/SharedSecretHandle.h
)

# 解析公共头文件路径
CHECK_FILE_PATHS("${SM_PUBLIC_HEADERS}" SM_PUBLIC_HEADERS_RESOLVED)

set(SM_PRIVATE_HEADERS
    src/dcps/topic/topicimpl.h
    src/dcps/topic/contentfilteredtopicimpl.h
    src/dcps/topic/multitopicimpl.h
    src/dcps/topic/topicdescriptionimpl.h
    src/dcps/base/entityimpl.h
    src/dcps/base/statusmanager.h
    src/dcps/condition/conditionimpl.h
    src/dcps/condition/statusconditionimpl.h
    src/dcps/condition/readconditionimpl.h
    src/dcps/condition/queryconditionimpl.h
    src/dcps/condition/guardconditionimpl.h
    src/dcps/condition/waitsetimpl.h
    src/dcps/publish/publisherimpl.h
    src/dcps/publish/datawriterimpl.h
    src/dcps/domain/domainentityimpl.h
    src/dcps/domain/domainparticipantimpl.h
    src/dcps/domain/domainparticipantfactoryimpl.h
    src/dcps/subscribe/datareaderimpl.h
    src/dcps/subscribe/subscriberimpl.h
    src/cdr/md5.h
    src/xml/tinyxml2.h
    src/xml/dds_xml_repo.h
    src/common/timeconversion.h
    src/common/memory/memorypool.h
    src/common/time/ddstimer.h
    src/common/log/logger.h
    src/common/threads/threadpool.h
    src/rtps/basictypertps.h
    src/rtps/messages/cdrmessage_t.h
    src/rtps/messages/parameter.h
    src/rtps/messages/messageprocessor.h
    src/rtps/transport/locator.h
    src/rtps/transport/messagetransmitterudp.h
    src/rtps/transport/ipcontainer.h
    src/rtps/transport/asiocontext.h
    src/rtps/transport/messagetransmitter.h
    src/rtps/endpoint/statefulreader.h
    src/rtps/endpoint/writer.h
    src/rtps/endpoint/statefulwriter.h
    src/rtps/endpoint/readerlocator.h
    src/rtps/endpoint/statelessreader.h
    src/rtps/endpoint/writerproxy.h
    src/rtps/endpoint/participant.h
    src/rtps/endpoint/endpoint.h
    src/rtps/endpoint/rtpsentity.h
    src/rtps/endpoint/statelesswriter.h
    src/rtps/endpoint/reader.h
    src/rtps/endpoint/changefromwriter.h
    src/rtps/endpoint/readerproxy.h
    src/rtps/endpoint/changeforreader.h
    src/rtps/historycache/historycache.h
    src/rtps/discovery/discovery.h
    src/rtps/discovery/spdp.h
    src/rtps/discovery/sedp.h
    src/rtps/discovery/discoverydata.h
    src/rtps/builtin/liveliness/livelinessmanager.h
    src/rtps/builtin/liveliness/livelinessassert.h
    src/rtps/builtin/liveliness/livelinesstatus.h
    src/security/propertyqospolicy.h
    src/qos/qospolicycheck.h
)

# 解析私有头文件路径
CHECK_FILE_PATHS("${SM_PRIVATE_HEADERS}" SM_PRIVATE_HEADERS_RESOLVED)

# 定义静态库目标
add_library(dds_security_sm STATIC
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
    target_link_libraries(dds_security_sm OpenSSL::SSL OpenSSL::Crypto)
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
    target_link_libraries(${test_name} 
    dds_security_sm 
    GTest::gtest 
    GTest::gtest_main
    OpenSSL::SSL 
    OpenSSL::Crypto
)
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

