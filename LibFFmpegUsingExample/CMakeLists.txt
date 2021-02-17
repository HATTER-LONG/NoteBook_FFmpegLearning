cmake_minimum_required(VERSION 3.5)

project(libav_tutorial)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_VERBOSE_MAKEFILE OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-Wall -pedantic)

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

add_executable(00_01_HelloWorld "${CMAKE_CURRENT_SOURCE_DIR}/Src/00_libav_tutoria/00_HelloWorld.cpp")

target_link_libraries(00_01_HelloWorld
    ${LOCAL_LINK_LIB} 
)

add_executable(00_02_Remuxing "${CMAKE_CURRENT_SOURCE_DIR}/Src/00_libav_tutoria/01_Remuxing.cpp")

target_link_libraries(00_02_Remuxing
    ${LOCAL_LINK_LIB} 
)

add_executable(00_03_Transcoding "${CMAKE_CURRENT_SOURCE_DIR}/Src/00_libav_tutoria/02_Transcoding.cpp"
                           "${CMAKE_CURRENT_SOURCE_DIR}/Src/00_libav_tutoria/video_debugging.cpp")

target_link_libraries(00_03_Transcoding
    ${LOCAL_LINK_LIB} 
)
set (CMAKE_C_COMPILER "/usr/bin/clang")
add_executable(01_01_FFmpegWithAudioBaseMac "${CMAKE_CURRENT_SOURCE_DIR}/Src/01_FFmpegUsingExample/01_FFmpegWithAudioBaseMac.c")
target_link_libraries(01_01_FFmpegWithAudioBaseMac)

add_executable(01_02_FFmpegCodecAACBaseMac "${CMAKE_CURRENT_SOURCE_DIR}/Src/01_FFmpegUsingExample/02_FFmpegCodecAACBaseMac.c")
target_link_libraries(01_02_FFmpegCodecAACBaseMac)

add_executable(01_03_FFmpegReadYUMBaseMac "${CMAKE_CURRENT_SOURCE_DIR}/Src/01_FFmpegUsingExample/03_FFmpegReadYUMBaseMac.c")
target_link_libraries(01_03_FFmpegReadYUMBaseMac)

add_executable(01_04_FFmpegReadNV12ToYum420BaseMac "${CMAKE_CURRENT_SOURCE_DIR}/Src/01_FFmpegUsingExample/04_FFmpegReadNV12ToYum420BaseMac.c")
target_link_libraries(01_04_FFmpegReadNV12ToYum420BaseMac)