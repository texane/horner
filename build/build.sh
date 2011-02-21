#!/usr/bin/env sh

XKAAPI_DIR="$HOME/install/xkaapi_master"
XKAAPI_CFLAGS="-DCONFIG_USE_XKAAPI=1 -I$XKAAPI_DIR/include"
XKAAPI_LFLAGS="-L$XKAAPI_DIR/lib -lkaapi -lpthread"

gcc \
    -Wall -O3 -march=native \
    $XKAAPI_CFLAGS \
    -o horner_modp \
    ../src/main_modp.c \
    -lm \
    $XKAAPI_LFLAGS
