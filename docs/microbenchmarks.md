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

## 02 - External Function

## 03 - x86_64 Division in Internal Code

## 03b - x86_64 Division in External Code

## 05 - ntohs

## 05 - strlen

## 06 - Overhead of the Context Switch
