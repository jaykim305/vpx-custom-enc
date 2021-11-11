cmake_minimum_required(VERSION 3.0.0)
project("custom vpx encoder" VERSION 0.1.0)

message("project name: ${CMAKE_PROJECT_NAME}")

set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")
add_compile_options(-Wall -g)

# include header files
include_directories(${CMAKE_SOURCE_DIR}/libvpx)
include_directories(${CMAKE_SOURCE_DIR}/libvpx/third_party/libyuv/include)
include_directories(${CMAKE_SOURCE_DIR}/libvpx/third_party/libwebm)

# include shared library
link_directories(${CMAKE_SOURCE_DIR}/libvpx) #notify shared library path
link_directories(${CMAKE_SOURCE_DIR}/src)
link_libraries(m pthread vpx) #libvpx.so

# include third party src not included in libvpx.so

add_executable( simple_encoder 
                ${CMAKE_SOURCE_DIR}/src/simple_encoder.c
                ${CMAKE_SOURCE_DIR}/libvpx/video_writer.c 
                ${CMAKE_SOURCE_DIR}/libvpx/tools_common.c 
                ${CMAKE_SOURCE_DIR}/libvpx/ivfenc.c 
                ${CMAKE_SOURCE_DIR}/libvpx/y4minput.c
)

add_executable( vpxenc 
                ${CMAKE_SOURCE_DIR}/src/vpxenc.c 
                ${CMAKE_SOURCE_DIR}/libvpx/args.c 
                ${CMAKE_SOURCE_DIR}/libvpx/tools_common.c 
                ${CMAKE_SOURCE_DIR}/libvpx/y4minput.c 
                ${CMAKE_SOURCE_DIR}/libvpx/ivfenc.c 
                ${CMAKE_SOURCE_DIR}/libvpx/video_writer.c 
                ${CMAKE_SOURCE_DIR}/libvpx/vpxstats.c 
                ${CMAKE_SOURCE_DIR}/libvpx/rate_hist.c 
                ${CMAKE_SOURCE_DIR}/libvpx/warnings.c 
                ${CMAKE_SOURCE_DIR}/libvpx/webmenc.cc
                ${CMAKE_SOURCE_DIR}/libvpx/third_party/libyuv/source/scale.cc
                ${CMAKE_SOURCE_DIR}/libvpx/third_party/libyuv/source/scale_common.cc
                ${CMAKE_SOURCE_DIR}/libvpx/third_party/libyuv/source/scale_any.cc
                ${CMAKE_SOURCE_DIR}/libvpx/third_party/libyuv/source/scale_gcc.cc
                ${CMAKE_SOURCE_DIR}/libvpx/third_party/libyuv/source/row_common.cc
                ${CMAKE_SOURCE_DIR}/libvpx/third_party/libyuv/source/row_any.cc
                ${CMAKE_SOURCE_DIR}/libvpx/third_party/libyuv/source/row_gcc.cc
                ${CMAKE_SOURCE_DIR}/libvpx/third_party/libyuv/source/planar_functions.cc
                ${CMAKE_SOURCE_DIR}/libvpx/third_party/libyuv/source/cpu_id.cc
                ${CMAKE_SOURCE_DIR}/libvpx/third_party/libwebm/common/hdr_util.cc 
                ${CMAKE_SOURCE_DIR}/libvpx/third_party/libwebm/mkvmuxer/mkvmuxer.cc
                ${CMAKE_SOURCE_DIR}/libvpx/third_party/libwebm/mkvmuxer/mkvmuxerutil.cc
                ${CMAKE_SOURCE_DIR}/libvpx/third_party/libwebm/mkvmuxer/mkvwriter.cc
                ${CMAKE_SOURCE_DIR}/libvpx/third_party/libwebm/mkvparser/mkvparser.cc
                ${CMAKE_SOURCE_DIR}/libvpx/third_party/libwebm/mkvparser/mkvreader.cc 
)

add_executable( my_vpxenc 
                ${CMAKE_SOURCE_DIR}/src/my_vpxenc.cpp
                ${CMAKE_SOURCE_DIR}/libvpx/args.c 
                ${CMAKE_SOURCE_DIR}/libvpx/tools_common.c 
                ${CMAKE_SOURCE_DIR}/libvpx/y4minput.c 
                ${CMAKE_SOURCE_DIR}/libvpx/ivfenc.c 
                ${CMAKE_SOURCE_DIR}/libvpx/video_writer.c 
                ${CMAKE_SOURCE_DIR}/libvpx/vpxstats.c 
                ${CMAKE_SOURCE_DIR}/libvpx/rate_hist.c 
                ${CMAKE_SOURCE_DIR}/libvpx/warnings.c 
                ${CMAKE_SOURCE_DIR}/libvpx/webmenc.cc
                ${CMAKE_SOURCE_DIR}/libvpx/third_party/libyuv/source/scale.cc
                ${CMAKE_SOURCE_DIR}/libvpx/third_party/libyuv/source/scale_common.cc
                ${CMAKE_SOURCE_DIR}/libvpx/third_party/libyuv/source/scale_any.cc
                ${CMAKE_SOURCE_DIR}/libvpx/third_party/libyuv/source/scale_gcc.cc
                ${CMAKE_SOURCE_DIR}/libvpx/third_party/libyuv/source/row_common.cc
                ${CMAKE_SOURCE_DIR}/libvpx/third_party/libyuv/source/row_any.cc
                ${CMAKE_SOURCE_DIR}/libvpx/third_party/libyuv/source/row_gcc.cc
                ${CMAKE_SOURCE_DIR}/libvpx/third_party/libyuv/source/planar_functions.cc
                ${CMAKE_SOURCE_DIR}/libvpx/third_party/libyuv/source/cpu_id.cc
                ${CMAKE_SOURCE_DIR}/libvpx/third_party/libwebm/common/hdr_util.cc 
                ${CMAKE_SOURCE_DIR}/libvpx/third_party/libwebm/mkvmuxer/mkvmuxer.cc
                ${CMAKE_SOURCE_DIR}/libvpx/third_party/libwebm/mkvmuxer/mkvmuxerutil.cc
                ${CMAKE_SOURCE_DIR}/libvpx/third_party/libwebm/mkvmuxer/mkvwriter.cc
                ${CMAKE_SOURCE_DIR}/libvpx/third_party/libwebm/mkvparser/mkvparser.cc
                ${CMAKE_SOURCE_DIR}/libvpx/third_party/libwebm/mkvparser/mkvreader.cc  )