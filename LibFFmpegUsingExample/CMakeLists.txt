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

add_executable(HelloWorld "${CMAKE_CURRENT_SOURCE_DIR}/Src/00_libav_tutoria/00_HelloWorld.cpp")

target_link_libraries(HelloWorld
    ${LOCAL_LINK_LIB} 
)

add_executable(Remuxing "${CMAKE_CURRENT_SOURCE_DIR}/Src/00_libav_tutoria/01_Remuxing.cpp")

target_link_libraries(Remuxing
    ${LOCAL_LINK_LIB} 
)

add_executable(Transcoding "${CMAKE_CURRENT_SOURCE_DIR}/Src/00_libav_tutoria/02_Transcoding.cpp"
                           "${CMAKE_CURRENT_SOURCE_DIR}/Src/00_libav_tutoria/video_debugging.cpp")

target_link_libraries(Transcoding
    ${LOCAL_LINK_LIB} 
)