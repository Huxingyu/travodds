cmake_minimum_required(VERSION 3.10)
project(persistence_example LANGUAGES CXX)

# 设置C++17标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找依赖
find_package(TRAVODDS 1 REQUIRED)
find_package(SQLite3 REQUIRED)

# 持久化服务库
add_library(persistence_service STATIC
    src/persistence/PersistenceService.cpp
)

target_include_directories(persistence_service 
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(persistence_service 
    PUBLIC 
    TRAVODDS::TRAVODDS
    SQLite::SQLite3
    pthread
)

# 持久化服务示例程序
add_executable(persistence_service_app
    example/persistence_service_main.cpp
)

target_link_libraries(persistence_service_app 
    PRIVATE 
    persistence_service
)

# 持久化Hello World示例
add_executable(persistent_hello_world 
    persistent_hello_world.cpp
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
    TRAVODDS::TRAVODDS
    pthread
)
