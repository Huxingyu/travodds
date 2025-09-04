cmake_minimum_required(VERSION 3.10)
project(persistence_example LANGUAGES CXX)

# 设置C++17标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找依赖
# find_package(TRAVODDS 1 REQUIRED)
# 直接使用本地构建的库
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../include)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../build/src)
find_package(SQLite3 REQUIRED)

# 持久化服务库
add_library(persistence_service STATIC
    src/PersistenceService.cpp
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

# 测试发布者 - 用于测试自动发现
add_executable(test_publisher
    example/test_publisher.cpp
    ../cpp/hello_world/HelloWorld.cpp
    ../cpp/hello_world/HelloWorldTypeSupport.cpp
)

target_include_directories(test_publisher 
    PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../cpp/hello_world
)

target_link_libraries(test_publisher 
    PRIVATE 
    travodds
    pthread
)
