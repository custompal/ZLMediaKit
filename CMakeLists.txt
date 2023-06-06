# MIT License
#
# Copyright (c) 2016-2022 The ZLMediaKit project authors. All Rights Reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

cmake_minimum_required(VERSION 3.13)

project(ZLMediaKit LANGUAGES C CXX)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/common.cmake)

##############################################################################

option(ENABLE_API "Enable C API SDK" OFF)
option(ENABLE_API_STATIC_LIB "Enable mk_api static lib" OFF)
option(ENABLE_CXX_API "Enable C++ API SDK" OFF)
option(ENABLE_HLS "Enable HLS" ON)
option(ENABLE_MP4 "Enable MP4" ON)
option(ENABLE_PLAYER "Enable Player" ON)
option(ENABLE_RTPPROXY "Enable RTPPROXY" ON)
option(ENABLE_SERVER "Enable Server" ON)
option(ENABLE_SERVER_LIB "Enable server as android static library" OFF)
option(ENABLE_SRT "Enable SRT" ON)
option(ENABLE_WEBRTC "Enable WebRTC" ON)
option(ENABLE_WEPOLL "Enable wepoll" ON)
option(DISABLE_REPORT "Disable report to report.zlmediakit.com" ON)

##############################################################################

#TODO: include 当前目录会导致 server 编译出错, 待排除
set(CMAKE_INCLUDE_CURRENT_DIR OFF)

string(TOLOWER ${CMAKE_HOST_SYSTEM_NAME} SYSTEM_NAME_LOWER)
#设置输出目录, 包括bin, lib以及其他文件
#Windows也不再区分32/64
set(OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/release/${SYSTEM_NAME_LOWER}/${BUILD_TYPE})
set(LIBRARY_OUTPUT_PATH ${OUTPUT_DIR})
set(EXECUTABLE_OUTPUT_PATH ${OUTPUT_DIR})

#mediakit以及各个runtime依赖
update_cached_list(MK_COMPILE_DEFINITIONS ${LIST_COMPILE_DEFINITIONS})
update_cached_list(MK_LINK_LIBRARIES ${LIST_LINK_LIBRARIES})

if (DISABLE_REPORT)
    update_cached_list(MK_COMPILE_DEFINITIONS DISABLE_REPORT)
endif ()

#开启ffmpeg
if (ENABLE_FFMPEG)
    include_directories(SYSTEM ${FFMPEG_INC_DIR})
    update_cached_list(MK_COMPILE_DEFINITIONS ENABLE_FFMPEG)
    update_cached_list(MK_LINK_LIBRARIES ${LIB_FFMPEG})
endif ()

#开启openssl
if (ENABLE_OPENSSL)
    include_directories(SYSTEM ${SSL_INC_DIR})
    update_cached_list(MK_COMPILE_DEFINITIONS ENABLE_OPENSSL)
    update_cached_list(MK_LINK_LIBRARIES ${LIB_OPENSSL})
else ()
    set(ENABLE_WEBRTC OFF)
    message(WARNING "openssl 未启用, rtmp 将不支持 flash 播放器, https/wss/rtsps/rtmps 也将失效")
endif ()

#开启mysql
if (ENABLE_MYSQL)
    include_directories(SYSTEM ${MYSQL_DIR_INC} ${MYSQL_INC_DIR}/mysql)
    update_cached_list(MK_COMPILE_DEFINITIONS ENABLE_MYSQL)
    update_cached_list(MK_LINK_LIBRARIES ${LIB_MYSQL})
endif ()

#开启x264
if (ENABLE_X264)
    include_directories(SYSTEM ${X264_INC_DIR})
    update_cached_list(MK_COMPILE_DEFINITIONS ENABLE_X264)
    update_cached_list(MK_LINK_LIBRARIES ${LIB_X264})
endif ()

#开启FAAC
if (ENABLE_FAAC)
    include_directories(SYSTEM ${FAAC_INC_DIR})
    update_cached_list(MK_COMPILE_DEFINITIONS ENABLE_FAAC)
    update_cached_list(MK_LINK_LIBRARIES ${LIB_FAAC})
endif ()

#链接jemalloc库避免内存碎片
if (ENABLE_JEMALLOC)
    include_directories(SYSTEM ${JEMALLOC_INCLUDE_DIR})
    update_cached_list(MK_LINK_LIBRARIES ${LIB_JEMALLOC})
endif ()

##############################################################################

#for assert.h
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdpart)

add_subdirectory(3rdpart)

add_subdirectory(src)

if (ENABLE_SRT)
    add_subdirectory(srt)
endif ()

if (ENABLE_WEBRTC)
    add_subdirectory(webrtc)
endif ()

if (ENABLE_API)
    add_subdirectory(api)
endif ()

#IOS不编译可执行程序
if (IOS)
    return()
endif ()

##############################################################################

if (ENABLE_PLAYER AND ENABLE_FFMPEG)
    add_subdirectory(player)
endif ()

#MediaServer主程序
if (ENABLE_SERVER)
    add_subdirectory(server)
endif ()

#Android会add_subdirectory并依赖该变量
if (ENABLE_SERVER_LIB)
    set(MK_LINK_LIBRARIES ${MK_LINK_LIBRARIES} PARENT_SCOPE)
endif ()

#cpp测试demo程序
if (ENABLE_TESTS)
    add_subdirectory(tests)
endif ()

#拷贝www文件夹、配置文件、默认证书
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/www" DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/conf/config.ini" DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/default.pem" DESTINATION ${EXECUTABLE_OUTPUT_PATH})

#依赖关系
add_dependencies(zltoolkit 3rdparty)

#工程分组
if (USE_SOLUTION_FOLDERS)
    set(ZLM_PROJECT zltoolkit zlmediakit jsoncpp)
    if (TARGET mpeg)
        list(APPEND ZLM_PROJECT mpeg)
    endif ()
    if (TARGET mov AND TARGET flv)
        list(APPEND ZLM_PROJECT mov flv)
    endif ()
    if (TARGET webrtc)
        list(APPEND ZLM_PROJECT webrtc)
    endif ()
    if (TARGET srt)
        list(APPEND ZLM_PROJECT srt)
    endif ()
    if (TARGET test_player)
        list(APPEND ZLM_PROJECT test_player)
    endif ()
    if (TARGET MediaServer)
        list(APPEND ZLM_PROJECT MediaServer)
    endif ()
    if (TARGET mk_api)
        list(APPEND ZLM_PROJECT api)
    endif ()
    foreach (PROJECT ${ZLM_PROJECT})
        set_property(TARGET ${PROJECT} PROPERTY FOLDER "zlmediakit")
    endforeach ()
endif ()
