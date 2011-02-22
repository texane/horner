#!/usr/bin/env sh

XKAAPI_DIR="$HOME/install/xkaapi_master"

for i in `seq 0 47`; do
    LD_LIBRARY_PATH=$XKAAPI_DIR/lib:$LD_LIBRARY_PATH \
    KAAPI_CPUSET=0:$i \
    ./var ;
done
