BINARY=./main
BINARY_ARGS=
BINARY_SEED=./input.dat
BINARY_ARGS_SEED=${BINARY_SEED}

build:
	clang lib.c -shared -o libA.so -fPIC
	../../../symcc-hybrid/build/symcc main.c -o main.symfusion -L. -lA
	../../../../symcc/build/symcc main.c -o main.symcc -L. -lA
	clang main.c -o main.symqemu -L. -lA

include ../Makefile-stdin.inc
