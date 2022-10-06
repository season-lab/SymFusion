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

Step-by-step instructions for compiling these components can be found inside the [`Dockerfile`](https://github.com/season-lab/SymFusion/blob/master/docker/Dockerfile).

