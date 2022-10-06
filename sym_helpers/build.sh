#!/bin/bash

SCRIPT=`realpath $0`
SCRIPTPATH=`dirname $SCRIPT`

export PATH="$SCRIPTPATH/../symcc-klee/build/:${PATH}:$SCRIPTPATH/../symcc-hybrid/build/:${PATH}"

clang -c wrappers.c -O1 -fPIC \
    -I../symqemu-hybrid/include/ \
    -I../symqemu-hybrid/ \
    `pkg-config --cflags glib-2.0` \
    -I../symqemu-hybrid/tcg/ \
    -I../symqemu-hybrid/tcg/i386/ \
    -I../symqemu-hybrid/target/i386/ \
    -I../symqemu-hybrid/x86_64-linux-user/ \
    -I../symqemu-hybrid/accel/tcg/ && \
\
symcc -shared fpu_helper.c cc_helper.c int_helper.c \
    wrappers.o -o libsymhelpers.so -O1 -fPIC \
    -I../symqemu-hybrid/include/ \
    -I../symqemu-hybrid/ \
    `pkg-config --cflags --libs glib-2.0` \
    -I../symqemu-hybrid/tcg/ \
    -I../symqemu-hybrid/tcg/i386/ \
    -I../symqemu-hybrid/target/i386/ \
    -I../symqemu-hybrid/x86_64-linux-user/ \
    -I../symqemu-hybrid/accel/tcg/
