BINARY=./main
BINARY_ARGS=
BINARY_SEED=./input.dat
BINARY_ARGS_SEED=${BINARY_SEED}

build:
	clang-10 lib.c -shared -o libA.so -fPIC
	../../../symcc-hybrid/build/symcc main.c -o main.symfusion -L. -lA
	../../../original/symcc/build/symcc main.c -o main.symcc -L. -lA
	clang-10 main.c -o main.symqemu -L. -lA

include ../Makefile-stdin.inc
