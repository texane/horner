#!/usr/bin/env sh

XKAAPI_DIR="$HOME/install/xkaapi_gpu"
XKAAPI_CFLAGS="-DCONFIG_USE_XKAAPI=1 -I$XKAAPI_DIR/include"
XKAAPI_LFLAGS="-L$XKAAPI_DIR/lib -lkaapi -lpthread"

gcc \
    -Wall -O3 -march=native \
    $XKAAPI_CFLAGS \
    ../src/main.c \
    -lm \
    $XKAAPI_LFLAGS
