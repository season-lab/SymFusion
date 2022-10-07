# Installation

## Docker container
A prebuilt container image is available on Docker Hub. You can pull and launch it with:
```
$ docker run -ti --rm ercoppa/symfusion
```

## Manual build
SymFusion has been tested on Ubuntu 20.04 x86_64. A manual installation requires to build:
 * our fork of SymCC 
 * our fork of SymQEMU (two times)
 * TCG Symbolic Helpers
 * SymCC Rust helper 

Download the source code:
```
$ git clone https://github.com/season-lab/SymFusion.git
$ cd SymFusion
$ git submodule init
$ git submodule update --recursive
$ cd symcc-hybrid
$ git submodule init
$ git submodule update --recursive
$ cd ..
```

To build our forks of SymCC and SymQEMU, you have to satisfy their dependencies:
- SymCC: e.g., on Ubuntu `sudo apt install git cargo clang-10 cmake g++ git libz3-dev llvm-10-dev llvm-10-tools ninja-build python2 python3-pip zlib1g-dev`
- SymQEMU: e.g., on Ubuntu `sudo apt build-dep qemu`

To build them you can use two scripts:
```
$ ./build-symcc-qsym.sh
$ ./build-symqemu.sh
```

Step-by-step instructions for compiling these components can be found inside the [`Dockerfile`](https://github.com/season-lab/SymFusion/blob/master/docker/Dockerfile).

## Tests

To check whether SymFusion is running as expected, you can run the [microbenchmarks](/microbenchmarks).
