#!/bin/sh

THIRD_PARTY_PATH=`cd $(dirname ../..);/bin/pwd`
FFMPEG_PATH=$THIRD_PARTY_PATH/ffmpeg/centos64
JEMALLOC_PATH=$THIRD_PARTY_PATH/jemalloc/centos64
OPENSSL_PATH=$THIRD_PARTY_PATH/openssl/centos64
MYSQL_PATH=$THIRD_PARTY_PATH/libmysql/centos64
SDL2_PATH=$THIRD_PARTY_PATH/sdl2/centos64
SRTP_PATH=$THIRD_PARTY_PATH/libsrtp/centos64
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

sed -i "1cset(PKG_CONFIG_FOUND false)" ./player/CMakeLists.txt
cd build/linux

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
-DENABLE_X264=false \
-DAVCODEC_INCLUDE_DIR=$FFMPEG_PATH/include \
-DAVCODEC_LIBRARY=$FFMPEG_PATH/lib/libavcodec-58.so \
-DAVUTIL_INCLUDE_DIR=$FFMPEG_PATH/include \
-DAVUTIL_LIBRARY=$FFMPEG_PATH/lib/libavutil-56.so \
-DSWRESAMPLE_INCLUDE_DIR=$FFMPEG_PATH/include \
-DSWRESAMPLE_LIBRARY=$FFMPEG_PATH/lib/libswresample-3.so \
-DJEMALLOC_INCLUDE_DIR=$JEMALLOC_PATH/include \
-DJEMALLOC_LIBRARY=$JEMALLOC_PATH/lib/libjemalloc.a \
-DOPENSSL_ROOT_DIR=$OPENSSL_PATH \
-DMYSQL_INCLUDE_DIR=$MYSQL_PATH/include \
-DMYSQL_LIBRARIES=$MYSQL_PATH/lib/libmysqlclient.a \
-DSDL2_INCLUDE_DIR=$SDL2_PATH/include \
-DSDL2_LIBRARY=$SDL2_PATH/lib64/libSDL2.a \
-DSRTP_INCLUDE_DIRS=$SRTP_PATH/include \
-DSRTP_LIBRARIES=$SRTP_PATH/lib/libsrtp2.a \
-DX264_INCLUDE_DIRS=$X264_PATH/include \
-DX264_LIBRARIES=$X264_PATH/lib/libx264.so.161 \
../..;check_error
make -j$THREAD_NUM;check_error

cd ../..
sed -i "1cfind_package(PkgConfig QUIET)" ./player/CMakeLists.txt
