# Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.16.3)

project(HelloWorldExample VERSION 1)


find_package(OpenSSL REQUIRED)
if(OpenSSL_FOUND)
	message(STATUS "Find OpenSSL.")
else()
	message(FATAL_ERROR "OpenSSL not found!")
endif()


# 设置包含目录 - 需要根据自环境配置
include_directories(
	/opt/agiros/loong/include/
	/opt/agiros/loong/include/fastcdr
	/opt/agiros/loong/include/fastrtps
	#/home/zhenrong/gxj/workspace_0114/FASTDDS/foonathan_memory_vendor/install/include
)

# 设置库目录 fastcdr、fastrtps、foonathan_memory_vendor和openssl的库目录
link_directories(
	/opt/agiros/loong/lib
	/home/zhenrong/gxj/workspace_0114/FASTDDS/foonathan_memory_vendor/install/lib
)

#Check C++11
include(CheckCXXCompilerFlag)
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANG OR
        CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    check_cxx_compiler_flag(-std=c++11 SUPPORTS_CXX11)
    if(NOT SUPPORTS_CXX11)
        message(FATAL_ERROR "Compiler doesn't support C++11")
    endif()
endif()

message(STATUS "Configuring HelloWorld example...")
file(GLOB HELLOWORLD_EXAMPLE_SOURCES_CXX "*.cxx")
file(GLOB HELLOWORLD_EXAMPLE_SOURCES_CPP "*.cpp")

add_executable(HelloWorldExample ${HELLOWORLD_EXAMPLE_SOURCES_CXX} ${HELLOWORLD_EXAMPLE_SOURCES_CPP})
target_compile_definitions(HelloWorldExample PRIVATE
    $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
    $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
)

# 依赖库 根据实际库名
target_link_libraries(HelloWorldExample PRIVATE OpenSSL::SSL OpenSSL::Crypto)
target_link_libraries(HelloWorldExample PRIVATE
	libfastcdr.so
	libfastrtps.so
	#libfoonathan_memory.so
	)
	
install(TARGETS HelloWorldExample
    RUNTIME DESTINATION examples/C++/HelloWorldExample/${BIN_INSTALL_DIR})
