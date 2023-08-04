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

#运行依赖库
list(APPEND DEPEND_DLLS ${OPENSSL_DLL})
if (LINUX AND ENABLE_X264)
  list(APPEND DEPEND_DLLS ${X264_DLL})
endif ()

file(GLOB MediaServer_SRC_LIST ./*.cpp ./*.h)

set(COMPILE_DEFINITIONS ${MK_COMPILE_DEFINITIONS})

if(ENABLE_SERVER_LIB)
  list(APPEND COMPILE_DEFINITIONS DISABLE_MAIN)
  add_library(MediaServer STATIC ${MediaServer_SRC_LIST})
  target_compile_definitions(MediaServer
    PRIVATE ${COMPILE_DEFINITIONS})
  target_compile_options(MediaServer
    PRIVATE ${DEFAULT_COMPILE_OPTIONS})
  target_link_libraries(MediaServer
    PRIVATE ${MK_LINK_LIBRARIES})
  update_cached_list(MK_LINK_LIBRARIES MediaServer)
  return()
endif()

# IOS 不编译可执行程序，只做依赖库
if(IOS)
  add_library(MediaServer STATIC ${MediaServer_SRC_LIST})
else()
  add_executable(MediaServer ${MediaServer_SRC_LIST})
endif()

target_compile_definitions(MediaServer
  PRIVATE ${COMPILE_DEFINITIONS})
target_compile_options(MediaServer
  PRIVATE ${DEFAULT_COMPILE_OPTIONS})

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  target_link_libraries(MediaServer -Wl,--start-group ${MK_LINK_LIBRARIES} -Wl,--end-group)
else()
  target_link_libraries(MediaServer ${MK_LINK_LIBRARIES})
endif()

copy_file(MediaServer ${DEPEND_DLLS})
install(TARGETS MediaServer DESTINATION ${INSTALL_PATH_RUNTIME})
