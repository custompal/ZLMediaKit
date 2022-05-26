#!/bin/sh

THIRD_PARTY_PATH=`cd $(dirname ../..);/bin/pwd`
FFMPEG_PATH=$THIRD_PARTY_PATH/ffmpeg/centos64
JEMALLOC_PATH=$THIRD_PARTY_PATH/jemalloc/centos64
OPENSSL_PATH=$THIRD_PARTY_PATH/openssl/centos64
MYSQL_PATH=$THIRD_PARTY_PATH/libmysql/centos64
SDL2_PATH=$THIRD_PARTY_PATH/sdl2/centos64
SRTP_PATH=$THIRD_PARTY_PATH/libsrtp/centos64
SCTP_PATH=$THIRD_PARTY_PATH/libsctp/centos64
X264_PATH=$THIRD_PARTY_PATH/libx264/centos64
INSTALL_PATH=`/bin/pwd`/centos64
THREAD_NUM=`grep 'processor' /proc/cpuinfo | sort -u | wc -l`

#echo $INSTALL_PATH
#if [ -x "$INSTALL_PATH" ]; then
#    rm -rf $INSTALL_PATH/*
#else
#    mkdir -p $INSTALL_PATH
#fi

check_error()
{
    code=$?
    if [ $code -ne 0 ];then
        echo -e "\033[1;31merror code=$code!\033[0m"
        cd $BASEDIR
        exit
    fi
}

mkdir -p build/linux
rm -rf build/linux/*

cd build/linux

export PKG_CONFIG_PATH=$FFMPEG_PATH/lib/pkgconfig:$SDL2_PATH/lib64/pkgconfig:$PKG_CONFIG_PATH

cmake \
-DENABLE_API=true \
-DENABLE_CXX_API=true \
-DENABLE_HLS=true \
-DENABLE_MP4=true \
-DENABLE_MYSQL=true \
-DENABLE_OPENSSL=true \
-DENABLE_PLAYER=true \
-DENABLE_RTPPROXY=true \
-DENABLE_SERVER=true \
-DENABLE_TESTS=true \
-DENABLE_WEBRTC=true \
-DENABLE_FFMPEG=true \
-DENABLE_X264=false \
-DJEMALLOC_INCLUDE_DIR=$JEMALLOC_PATH/include \
-DJEMALLOC_LIBRARY=$JEMALLOC_PATH/lib/libjemalloc.a \
-DOPENSSL_ROOT_DIR=$OPENSSL_PATH \
-DMYSQL_INCLUDE_DIR=$MYSQL_PATH/include \
-DMYSQL_LIBRARIES=$MYSQL_PATH/lib/libmysqlclient.a \
-DSRTP_INCLUDE_DIRS=$SRTP_PATH/include \
-DSRTP_LIBRARIES=$SRTP_PATH/lib/libsrtp2.a \
-DSCTP_INCLUDE_DIRS=$SCTP_PATH/include \
-DSCTP_LIBRARIES=$SCTP_PATH/lib64/libusrsctp.a \
-DX264_INCLUDE_DIRS=$X264_PATH/include \
-DX264_LIBRARIES=$X264_PATH/lib/libx264.so.161 \
../..;check_error
make -j$THREAD_NUM;check_error

cd ../..
