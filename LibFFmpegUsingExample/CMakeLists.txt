cmake_minimum_required(VERSION 3.5)

project(libav_tutorial LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_VERBOSE_MAKEFILE OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-Wall -pedantic -Wc99-extensions)

find_package(Threads)
find_package(spdlog REQUIRED CONFIG)
set(LOCAL_LINK_LIB spdlog::spdlog pthread)
## 指定头文件、库查找路径
include_directories(Include Src)
link_directories(Lib)
link_libraries(# 注意包的依赖顺序
    avformat
    avdevice
    avcodec
    avfilter
    swresample
    swscale
    avutil
    -lx264
    -lx265
    -lz 
)

add_executable(HelloWorld "${CMAKE_CURRENT_SOURCE_DIR}/HelloWorld.cpp")

target_link_libraries(HelloWorld
    ${LOCAL_LINK_LIB} 
)

add_executable(remuxing "${CMAKE_CURRENT_SOURCE_DIR}/remuxing.cpp")

target_link_libraries(remuxing
    ${LOCAL_LINK_LIB} 
)