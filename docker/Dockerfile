# This file is part of SymCC.
#
# SymCC is free software: you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# SymCC is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# SymCC. If not, see <https://www.gnu.org/licenses/>.

#
# The build stage
#
FROM ubuntu:20.04 AS builder

RUN cp /etc/apt/sources.list /etc/apt/sources.list.bak
RUN sed -Ei 's/^# deb-src /deb-src /' /etc/apt/sources.list

# Install dependencies
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    cargo \
    clang-10 \
    clang \
    cmake \
    g++ \
    git wget \
    libz3-dev \
    llvm-10-dev \
    llvm-10-tools \
    ninja-build \
    python2 \
    python3-pip \
    zlib1g-dev \
    ca-certificates \
    && DEBIAN_FRONTEND=noninteractive apt build-dep -y qemu && update-ca-certificates \
    && rm -rf /var/lib/apt/lists/*
RUN pip3 install lit

RUN ln -s /usr/bin/llvm-config-10 /usr/bin/llvm-config

# Download the LLVM sources already so that we don't need to get them again when
# SymCC changes
RUN git clone -b llvmorg-10.0.1 --depth 1 https://github.com/llvm/llvm-project.git /llvm_source

# Build new Z3 (old one has a regression)
WORKDIR /z3
RUN wget https://github.com/Z3Prover/z3/archive/refs/tags/z3-4.8.14.tar.gz
RUN tar xvf z3-4.8.14.tar.gz
RUN mkdir /symfusion
RUN cp -a z3-z3-4.8.14 /symfusion/z3
WORKDIR /symfusion/z3/build
RUN cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=`pwd`/dist && make -j `nproc` && make install

# Build original SymCC with the QSYM backend
WORKDIR /symfusion/original/
RUN git clone https://github.com/eurecom-s3/symcc.git && cd symcc && git checkout 9b20609ada && git submodule init && git submodule update

# DISABLE FUNCTIONS MODELS IN SYMCC
# COPY ./utils/compiler_runtime.cpp /symfusion/original/symcc/compiler/Runtime.cpp
# COPY ./utils/compiler_symbolizer.cpp /symfusion/original/symcc/compiler/Symbolizer.cpp

# ADD FLAG TO SKIP QUERY
COPY ./utils/qsym_solver.cpp /symfusion/original/symcc/runtime/qsym_backend/qsym/qsym/pintool/solver.cpp

WORKDIR /symfusion/original/symcc/build
RUN git diff
RUN cmake -G Ninja \
    -DQSYM_BACKEND=ON \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DZ3_DIR=/symfusion/z3/build/dist/lib/cmake/z3 \
    -DLLVM_DIR=`llvm-config-10 --cmakedir` \
    /symfusion/original/symcc/ \
    && ninja

# This does not work with AFL++
# RUN cargo install --path /symfusion/original/symcc/util/symcc_fuzzing_helper

# Build original SymQEMU
WORKDIR /symfusion/original
RUN git clone https://github.com/eurecom-s3/symqemu.git
WORKDIR /symfusion/original/symqemu
RUN sed -Ei 's#https://git.qemu.org/git#https://gitlab.com/qemu-project#' .git/config
RUN sed -Ei 's#https://git.qemu.org/git#https://gitlab.com/qemu-project#' .gitmodules
RUN git checkout d183844 && ./configure                             \
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
    --symcc-source=/symfusion/original/symcc/                       \
    --symcc-build=/symfusion/original/symcc/build &&                \
    make -j `nproc`

# Build libc++ with SymCC
WORKDIR /symfusion/original/libcxx
RUN export SYMCC_REGULAR_LIBCXX=yes SYMCC_NO_SYMBOLIC_INPUT=yes \
     && mkdir /symfusion/original/libcxx_build \
     && cd /symfusion/original/libcxx_build \
     && cmake -G Ninja /llvm_source/llvm \
          -DLLVM_ENABLE_PROJECTS="libcxx;libcxxabi" \
          -DLLVM_TARGETS_TO_BUILD="X86" \
          -DLLVM_DISTRIBUTION_COMPONENTS="cxx;cxxabi;cxx-headers" \
          -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_INSTALL_PREFIX=/symfusion/original/libcxx_install \
          -DCMAKE_C_COMPILER=/symfusion/original/symcc/build/symcc \
          -DCMAKE_CXX_COMPILER=/symfusion/original/symcc/build/sym++ \
     && ninja distribution \
     && ninja install-distribution

# Init submodules if they are not initialiazed yet
COPY ./symcc-hybrid /symfusion/symcc-hybrid
COPY ./symqemu-hybrid /symfusion/symqemu-hybrid
COPY ./.git /symfusion/.git

WORKDIR /symfusion/symcc-hybrid
RUN $(rm -r /symfusion/symcc-hybrid/build/ || echo "")

# Build SymCC-hybrid with the QSYM backend
WORKDIR /symfusion/symcc-hybrid/build
RUN cmake -G Ninja \
    -DQSYM_BACKEND=ON \
    -DCMAKE_BUILD_TYPE=Release \
    -DZ3_DIR=/symfusion/z3/build/dist/lib/cmake/z3 \
    -DLLVM_DIR=`llvm-config-10 --cmakedir` \
    /symfusion/symcc-hybrid/ \
    && ninja
RUN cargo install --path /symfusion/symcc-hybrid/util/symcc_fuzzing_helper

# Build libc++ with symfusion
WORKDIR /symfusion/libcxx
RUN export SYMCC_REGULAR_LIBCXX=yes SYMCC_NO_SYMBOLIC_INPUT=yes \
     && mkdir /symfusion/libcxx_build \
     && cd /symfusion/libcxx_build \
     && cmake -G Ninja /llvm_source/llvm \
          -DLLVM_ENABLE_PROJECTS="libcxx;libcxxabi" \
          -DLLVM_TARGETS_TO_BUILD="X86" \
          -DLLVM_DISTRIBUTION_COMPONENTS="cxx;cxxabi;cxx-headers" \
          -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_INSTALL_PREFIX=/symfusion/libcxx_install \
          -DCMAKE_C_COMPILER=/symfusion/symcc-hybrid/build/symcc \
          -DCMAKE_CXX_COMPILER=/symfusion/symcc-hybrid/build/sym++ \
     && ninja distribution \
     && ninja install-distribution

WORKDIR /symfusion/symqemu-hybrid
RUN sed -Ei 's#https://git.qemu.org/git#https://gitlab.com/qemu-project#' .gitmodules
RUN CFLAGS="-march=ivybridge" ./configure                           \
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
    --symcc-source=/symfusion/symcc-hybrid/                         \
    --symcc-build=/symfusion/symcc-hybrid/build &&                  \
    make clean &&                                                   \
    CFLAGS="-march=ivybridge" make -j

COPY ./sym_helpers /symfusion/sym_helpers
WORKDIR /symfusion/sym_helpers
RUN ./build.sh

WORKDIR /symfusion/symqemu-hybrid
RUN CFLAGS="-march=ivybridge" ./configure                           \
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
    --symcc-source=/symfusion/symcc-hybrid/                         \
    --symcc-build=/symfusion/symcc-hybrid/build &&                  \
    make clean &&                                                   \
    CFLAGS="-march=ivybridge" make -j

# bitmap comparer
COPY ./utils/ /symfusion/utils
WORKDIR /symfusion/utils
RUN make comparer

#
# The final image
#
FROM ubuntu:20.04

# libtool-bin bison
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    build-essential \
    clang-10 clang-10++ llvm-dev \
    g++ \
    libllvm10 \
    zlib1g \
    sudo \
    nano \
    libglib2.0-dev \
    gdb \
    wget \
    curl \
    time \
    python3-pip ninja-build \
    build-essential automake autoconf texinfo flex bison gcovr \
    libtool libjpeg-dev libz-dev liblzma-dev \
    libsqlite3-dev sqlite3 \
    git make autoconf automake libtool bison re2c pkg-config libicu-dev \
    git make autoconf automake libtool pkg-config cmake zlib1g-dev libjpeg-dev libopenjp2-7-dev libpng-dev libcairo2-dev libtiff-dev liblcms2-dev libboost-dev \
    git make autoconf autogen automake build-essential libasound2-dev libflac-dev libogg-dev libtool libvorbis-dev libopus-dev libmp3lame-dev libmpg123-dev pkg-config python \
    && rm -rf /var/lib/apt/lists/* 

RUN groupadd --gid 1008 ubuntu && \
     useradd --uid 1008 --gid ubuntu --shell /bin/bash --create-home ubuntu && \
     echo 'ubuntu ALL=(ALL) NOPASSWD:ALL' > /etc/sudoers.d/ubuntu

# Build AFL++
RUN git clone https://github.com/AFLplusplus/AFLplusplus.git /afl && cd /afl && git checkout 3.14c
# COPY ./AFLplusplus /afl
WORKDIR /afl
COPY --from=builder /symfusion/utils/afl-showmap.c src
RUN make clean && make -j `nproc`
RUN cd qemu_mode && ./build_qemu_support.sh
# RUN cd custom_mutators/concolic && make clean && make

USER ubuntu

WORKDIR /home/ubuntu
RUN wget -q https://github.com/hugsy/gef/raw/master/scripts/gef.sh 
RUN chmod +x ./gef.sh
RUN bash -c ./gef.sh

RUN pip install psutil

COPY --from=builder /symfusion/symcc-hybrid /symfusion/symcc-hybrid/
COPY --from=builder /symfusion/original/symcc/build /symfusion/original/symcc/build
COPY --from=builder /symfusion/symqemu-hybrid /symfusion/symqemu-hybrid
COPY --from=builder /symfusion/original/symqemu /symfusion/original/symqemu
COPY --from=builder /symfusion/sym_helpers /symfusion/sym_helpers
COPY --from=builder /root/.cargo/bin/symcc_fuzzing_helper /symfusion/symcc-hybrid/build/
COPY --from=builder /symfusion/symcc-hybrid/util/pure_concolic_execution.sh /symfusion/symcc-hybrid/build/
COPY --from=builder /symfusion/original/symcc/util/pure_concolic_execution.sh /symfusion/original/symcc/build/
COPY ./runner /symfusion/runner
COPY --from=builder /symfusion/z3 /symfusion/z3
COPY --from=builder /symfusion/libcxx_install /symfusion/libcxx_install
COPY --from=builder /symfusion/original/libcxx_install /symfusion/original/libcxx_install
COPY --from=builder /symfusion/utils /symfusion/utils

ENV PATH /symfusion/symcc-hybrid/build:$PATH
ENV AFL_PATH /afl

WORKDIR /symfusion/
