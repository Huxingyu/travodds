cmake_minimum_required(VERSION 3.5.0)

# 设置变量，给包含该.cmake的CMakeListes.txt使用

set(travoddsz_DIR "${CMAKE_CURRENT_LIST_DIR}")

set(travoddsz_DEFINITIONS _XML_INTERFACE_CPP _DDS_STATIC_LIB)

set(travoddsz_INCLUDE_DIRS "${travoddsz_DIR}/../include")

if (MSVC)
set(travoddszd_LIBRARIES "${travoddsz_DIR}/../lib/travoddszd.lib")
set(travoddsz_LIBRARIES "${travoddsz_DIR}/../lib/travoddsz.lib")
elseif (MINGW)
set(travoddszd_LIBRARIES "${travoddsz_DIR}/../lib/travoddszd.lib")
set(travoddsz_LIBRARIES "${travoddsz_DIR}/../lib/travoddsz.lib")
else()
set(travoddszd_LIBRARIES "${travoddszd_DIR}/../lib/libtravoddszd.a")
set(travoddsz_LIBRARIES "${travoddsz_DIR}/../lib/libtravoddsz.a")
endif()
