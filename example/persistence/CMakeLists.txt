cmake_minimum_required(VERSION 3.10)
project(persistence_example LANGUAGES CXX)

# 设置C++17标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找Qt5
find_package(Qt5 COMPONENTS Core Widgets QUIET)

# 查找依赖
# find_package(TRAVODDS 1 REQUIRED)
# 直接使用本地构建的库
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/tao
)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../build/src)
find_package(SQLite3 REQUIRED)

# 持久化服务库
add_library(persistence_service STATIC
    src/PersistenceService.cpp
    src/TypeSupportRegistry.cpp
    src/MemoryCache.cpp
    src/DatabaseManager.cpp
    src/DurabilityHandlers.cpp
    src/PersistenceTypes.cpp
)

target_include_directories(persistence_service 
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(persistence_service 
    PUBLIC 
    travoddsd
    SQLite::SQLite3
    pthread
)

# 持久化服务示例程序
add_executable(persistence_service_app
    example/persistence_service_main.cpp
    ../cpp/hello_world/HelloWorld.cpp
    ../cpp/hello_world/HelloWorldTypeSupport.cpp
)

target_include_directories(persistence_service_app 
    PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../cpp/hello_world
)

target_link_libraries(persistence_service_app 
    PRIVATE 
    persistence_service
)

# 条件性地添加Qt支持到持久化服务程序
if(Qt5_FOUND)
    message(STATUS "Found Qt5, adding GUI support to persistence_service_app")
    
    # 启用Qt的MOC、UIC、RCC
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)
    
    # 先创建持久化服务应用基础目标
    target_sources(persistence_service_app PRIVATE
        src/PersistenceMonitor.cpp
        include/PersistenceMonitor.h
    )
    
    # 创建一个单独的MOC目标处理头文件
    qt5_wrap_cpp(MONITOR_MOC_SOURCES 
        include/PersistenceMonitor.h
    )
    
    # 添加MOC生成的文件
    target_sources(persistence_service_app PRIVATE ${MONITOR_MOC_SOURCES})
    
    # 添加Qt头文件目录
    target_include_directories(persistence_service_app PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    
    # 链接Qt库
    target_link_libraries(persistence_service_app PRIVATE
        Qt5::Core
        Qt5::Widgets
    )
    
    # 定义BUILD_WITH_QT宏
    target_compile_definitions(persistence_service_app PRIVATE BUILD_WITH_QT)
    
    # 设置Qt应用程序属性
    if(WIN32)
        set_target_properties(persistence_service_app PROPERTIES
            WIN32_EXECUTABLE TRUE
        )
    endif()
    
else()
    message(WARNING "Qt5 not found, building console-only persistence service. Install Qt5 development packages to enable GUI support.")
endif()

# 持久化Hello World示例
add_executable(persistent_hello_world 
    example/persistent_hello_world.cpp
    ../cpp/hello_world/HelloWorld.cpp
    ../cpp/hello_world/HelloWorldTypeSupport.cpp
)

target_include_directories(persistent_hello_world 
    PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../cpp/hello_world
)

target_link_libraries(persistent_hello_world 
    PRIVATE 
    travoddsd
    pthread
)


