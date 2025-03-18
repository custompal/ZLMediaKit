# - Try to find sctp
#
# Once done this will define
#  SCTP_FOUND        - System has mbedtls
#  SCTP_INCLUDE_DIRS - The mbedtls include directories
#  SCTP_LIBRARIES    - The mbedtls library


if(NOT _SCTP_ROOT_PATHS)
set(_SCTP_ROOT_PATHS ${CMAKE_INSTALL_PREFIX})
endif()

#find Mbedtls
FIND_PATH(
    SCTP_INCLUDE_DIRS
    NAMES usrsctp.h
    HINTS ${_SCTP_ROOT_PATHS}  ${SCTP_PREFIX}
    PATH_SUFFIXES include
)

FIND_LIBRARY(
    SCTP_LIBRARIES
    NAMES usrsctp
    HINTS ${_SCTP_ROOT_PATHS} ${SCTP_PREFIX}
    PATH_SUFFIXES bin lib
)

message(STATUS "SCTP LIBRARIES: " ${SCTP_LIBRARIES})
message(STATUS "SCTP INCLUDE DIRS: " ${SCTP_INCLUDE_DIRS})

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SCTP DEFAULT_MSG SCTP_LIBRARIES SCTP_INCLUDE_DIRS)
