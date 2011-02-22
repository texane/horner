#!/usr/bin/env sh

XKAAPI_DIR="$HOME/install/xkaapi_master"
XKAAPI_CFLAGS="-I$XKAAPI_DIR/include"
XKAAPI_LFLAGS="-L$XKAAPI_DIR/lib -lkaapi -lpthread"

g++ \
    -Wall -O3 -march=native \
    $XKAAPI_CFLAGS \
    -I../../src \
    -o var \
    ../src/main.cc \
    $XKAAPI_LFLAGS
