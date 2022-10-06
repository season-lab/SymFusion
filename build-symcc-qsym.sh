#!/bin/bash

cd symcc-hybrid
rm -rf build
mkdir build 2>/dev/null
# RelWithDebInfo
cd build && cmake -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DQSYM_BACKEND=ON \
    -DZ3_DIR=/mnt/ssd/symbolic/symfusion/z3/build/dist/lib/cmake/z3 \
    .. && ninja

exit 0
