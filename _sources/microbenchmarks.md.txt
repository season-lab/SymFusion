# Microbenchmarks

We assume you are using our docker container image to run the microbenchmarks. E.g., started with:
```
docker run -ti --rm ercoppa/symfusion
```
If you are not running the docker container, you may have to fix a few paths in the `Makefile`s that we will use. 

Each microbenchmark is a different C program composed by `main.c` (internal code) and `lib.c` (external code). Look at `main.c` to get an idea of what the microbenchmark is doing.

## 01 - CPU Intensive Loop

The goal of this program is to assess the analysis overhead of SymFusion compared to SymCC and SymQEMU when using a CPU intensive loop.

```
$ cd tests/microbenchmarks/01-cpu-intensive-loop
$ make              # to build the program for SymFusion/SymCC/SymQEMU
$ make symfusion    # run the program under SymFusion
$ make symcc        # run the program under SymFusion
$ make symqemu      # run the program under SymFusion
```

From the output of the three tools, you can see their running time. On our machine:
 - SymFusion: 940 ms
 - SymCC: 910 ms
 - SymQEMU: 6008 ms

Hence, you can see that SymFusion is almost as fast as SymCC and way faster than SymQEMU.

## 02 - Data Flow via External Function

The goal of this program is to validate that SymFusion can correctly track a data flow even when this flow is happening inside a library function that was not instrumented at compilation time.

```
$ cd tests/microbenchmarks/02-external-function
$ make              # to build the program for SymFusion/SymCC/SymQEMU
$ make symfusion    # run the program under SymFusion
$ make symcc        # run the program under SymFusion
$ make symqemu      # run the program under SymFusion
```

Expected results:
 - SymFusion is able to generate an input to satify the branch inside the program. Check it with:
```
$ LD_LIBRARY_PATH=`pwd` ./main.symqemu < out/symfusion-00000000/000000
OK
```
where `OK` means that the branch was satisfied. `out` is the output directory used by SymFusion. We use the `main.symqemu` binary because it is binary without instrumentation (hence, no noise from SymCC or SymFusion).
 - SymCC does not generate any input able to satisfy the branch. It is missing the data flow via the external code. Check `out_symcc` to see that there no inputs.
 - SymQEMU is able to generate an input to satify the branch inside the program. Check it with:
```
$ LD_LIBRARY_PATH=`pwd` ./main.symqemu < out_symqemu/000000
OK
```
where `out_symqemu` is the output directory used by SymQEMU.

## 03 - x86_64 Division in Internal Code

The goal of this program is to validate whether SymFusion can handle an x84_64 division in the internal code.

```
$ cd tests/microbenchmarks/03-x86-division
$ make              # to build the program for SymFusion/SymCC/SymQEMU
$ make symfusion    # run the program under SymFusion
$ make symcc        # run the program under SymFusion
$ make symqemu      # run the program under SymFusion
```

Expected results:
 - SymFusion is able to generate an input to satify the branch inside the program. Check it with:
```
$ LD_LIBRARY_PATH=`pwd` ./main.symqemu < out/symfusion-00000000/000000
OK
```
where `OK` means that the branch was satisfied. `out` is the output directory used by SymFusion.
 - SymCC is able to generate an input to satify the branch inside the program. Check it with:
```
$ LD_LIBRARY_PATH=`pwd` ./main.symqemu < out_symcc/000000
OK
```
where `out_symcc` is the output directory used by SymCC.
 - SymQEMU does not generate any input able to satisfy the branch. Indeed, SymQEMU does not symbolically reason on the QEMU helpers used to handle the x86_64 division. Notice that SymQEMU generates one input in `out_symqemu` but this is not the correct one.

## 03b - x86_64 Division in External Code

The goal of this program is to validate whether SymFusion can handle an x84_64 division in the external code.

```
$ cd tests/microbenchmarks/03-x86-division-b
$ make              # to build the program for SymFusion/SymCC/SymQEMU
$ make symfusion    # run the program under SymFusion
$ make symcc        # run the program under SymFusion
$ make symqemu      # run the program under SymFusion
```

Expected results:
 - SymFusion is able to generate an input to satify the branch inside the program. Check it with:
```
$ LD_LIBRARY_PATH=`pwd` ./main.symqemu < out/symfusion-00000000/000000
OK
```
where `OK` means that the branch was satisfied. `out` is the output directory used by SymFusion.
 - SymCC does not generate any input able to satisfy the branch. Indeed, it is ignoring the symbolic operation in the external code.
 - SymQEMU does not generate any input able to satisfy the branch. Indeed, SymQEMU does not symbolically reason on the QEMU helpers used to handle the x86_64 division. Notice that SymQEMU generates one input in `out_symqemu` but this is not the correct one.

## 05 - ntohs

The goal of this program is to validate whether SymFusion can handle `ntohs` from the standard C library.

```
$ cd tests/microbenchmarks/04-ntohs
$ make              # to build the program for SymFusion/SymCC/SymQEMU
$ make symfusion    # run the program under SymFusion
$ make symcc        # run the program under SymFusion
$ make symqemu      # run the program under SymFusion
```

Expected results:
 - SymFusion is able to generate an input to satify the branch inside the program. Check it with:
```
$ LD_LIBRARY_PATH=`pwd` ./main.symqemu < out/symfusion-00000000/000000
OK
```
where `OK` means that the branch was satisfied. `out` is the output directory used by SymFusion.
 - SymCC does not generate any input able to satisfy the branch. `ntohs` is not modelled by SymCC and thus its effects are ignored.
 - SymQEMU is able to generate an input to satify the branch inside the program. Check it with:
```
$ LD_LIBRARY_PATH=`pwd` ./main.symqemu < out_symqemu/000000
OK
```
where `out_symqemu` is the output directory used by SymQEMU.

If you try to change `ntohs` with `ntohl` in `main.c` and then recompile the program and run again the tools, you can see that SymCC can generate a valid input since it has a model. The main is that SymFusion can track the code of functions even without models for them.

## 05 - strlen

The goal of this program is to validate whether SymFusion can handle `strlen` from the standard C library.

```
$ cd tests/microbenchmarks/05-strlen
$ make              # to build the program for SymFusion/SymCC/SymQEMU
$ make symfusion    # run the program under SymFusion
$ make symcc        # run the program under SymFusion
$ make symqemu      # run the program under SymFusion
```

Expected results:
 - SymFusion is able to generate an input to satify the branch inside the program. Check it with:
```
$ LD_LIBRARY_PATH=`pwd` ./main.symqemu < out/symfusion-00000000/000000
OK
```
where `OK` means that the branch was satisfied. `out` is the output directory used by SymFusion. However, this is possible thanks the use of a function model. If you disable the function model for `strlen` in SymFusion, it will accurately reason on `strlen`, however, the resulting expressions are too complex: the binary code is likely using vectorized instructions which require a concolic executor to handle symbolic addresses to solve them in some cases. SymFusion, similarly to SymCC and SymQEMU, concretizes symbolic addresses. 
 - SymCC does not generate any input able to satisfy the branch. `strlen` is not modelled by SymCC and thus its effects are ignored.
 - SymQEMU does not generate any input able to satisfy the branch. Indeed, `strlen` is likely implemented with vectorized instructions that in QEMU are handled by helpers but SymQEMU ignores the effects of the QEMU helpers.

## 06 - Overhead of the Context Switch

The goal of this program is to assess the cost of the context switch between the two execution modes (native mode and virtual mode) used by SymFusion.

```
$ cd tests/microbenchmarks/06-cost-context-switch
$ make              # to build the program for SymFusion/SymCC/SymQEMU
$ make symfusion    # run the program under SymFusion
$ make symcc        # run the program under SymFusion
$ make symqemu      # run the program under SymFusion
```

From the output of the three tools, you can see their running time. On our machine:
 - SymFusion: 114 ms
 - SymCC: 11 ms
 - SymQEMU: 84 ms

We can that SymFusion is way slower than SymCC and even a bit slower than SymQEMU. This is due to large number of context switches (15000) performed by the program. 