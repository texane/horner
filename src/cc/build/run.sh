#!/usr/bin/env sh

XKAAPI_DIR="$HOME/install/xkaapi_master"

for i in `seq 0 4 48`; do
    LD_LIBRARY_PATH=$XKAAPI_DIR/lib:$LD_LIBRARY_PATH \
    LD_PRELOAD=$HOME/install/lib/libtcmalloc.so \
    KAAPI_CPUSET=`seq -s, 0 4 $i` \
    numactl --interleave=all \
    ./horner_modp ;
done
