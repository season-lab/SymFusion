#!/bin/bash

cd symqemu-hybrid
CFLAGS="-march=ivybridge" ./configure                           \
    --audio-drv-list=                                               \
    --disable-bluez                                                 \
    --disable-sdl                                                   \
    --disable-gtk                                                   \
    --disable-vte                                                   \
    --disable-opengl                                                \
    --disable-virglrenderer                                         \
    --disable-werror                                                \
    --target-list=x86_64-linux-user                                 \
    --enable-capstone=git                                           \
    --symcc-source=`pwd`/../symcc-hybrid/                           \
    --symcc-build=`pwd`/../symcc-hybrid/build &&                    \
    make clean &&                                                   \
    CFLAGS="-march=ivybridge" make -j `nproc`

cd ../sym_helpers
./build.sh 

cd ../symqemu-hybrid
CFLAGS="-march=ivybridge" ./configure                           \
    --audio-drv-list=                                               \
    --disable-bluez                                                 \
    --disable-sdl                                                   \
    --disable-gtk                                                   \
    --disable-vte                                                   \
    --disable-opengl                                                \
    --disable-virglrenderer                                         \
    --disable-werror                                                \
    --target-list=x86_64-linux-user                                 \
    --enable-capstone=git                                           \
    --symcc-source=`pwd`/../symcc-hybrid/                           \
    --symcc-build=`pwd`/../symcc-hybrid/build &&                    \
    make clean &&                                                   \
    CFLAGS="-march=ivybridge" make -j `nproc`