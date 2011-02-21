#!/usr/bin/env sh

XKAAPI_DIR="$HOME/install/xkaapi_master"

LD_LIBRARY_PATH=$XKAAPI_DIR/lib:$LD_LIBRARY_PATH \
KAAPI_CPUSET=0,1,2,3,4,5,6,7 ./horner_modp
