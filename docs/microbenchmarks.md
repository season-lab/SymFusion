# Microbenchmarks

We assume you are using our docker container image to run the microbenchmarks. E.g., started with:
```
docker run -ti --rm ercoppa/symfusion
```
If you are not running the docker container, you may have to fix a few paths in the `Makefile`s that we will use.

## 01 - CPU Intensive Loop

The goal of this program is assess the analysis overhead of SymFusion compared to SymCC and SymQEMU when using a CPU intensive loop.

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

The goal of this program is validate that SymFusion can correctly track a data flow even when this flow is happening inside a library function that was not instrumented at compilation time.

```
$ cd tests/microbenchmarks/01-cpu-intensive-loop
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
 - SymCC does not generate any input able to satisfy the branch. It is missing the data flow via the external code. Check `out_symcc` to see that there no inpits.
 - SymQEMU is able to generate an input to satify the branch inside the program. Check it with:
```
$ LD_LIBRARY_PATH=`pwd` ./main.symqemu < out_symqemu/000000
OK
```
where `out_symqemu` is the output directory used by SymQEMU.

## 03 - x86_64 Division in Internal Code

## 03b - x86_64 Division in External Code

## 05 - ntohs

## 05 - strlen

## 06 - Overhead of the Context Switch
