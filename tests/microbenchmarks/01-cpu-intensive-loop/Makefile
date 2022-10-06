BINARY=./main
BINARY_ARGS=
BINARY_SEED=./input.dat
BINARY_ARGS_SEED=${BINARY_SEED}

build:
	../../../symcc-hybrid/build/symcc main.c -o main.symfusion
	../../../../symcc/build/symcc main.c -o main.symcc
	clang main.c -o main.symqemu

include ../Makefile-stdin.inc
