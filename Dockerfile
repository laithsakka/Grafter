FROM ubuntu:18.04

RUN apt-get update -y && \
    apt-get install -y wget git \
        gcc g++ automake autoconf make

RUN mkdir /build
WORKDIR /build

# Current Master on both [2018.11.04]:
ENV LLVM_SHA 88f96230a8449abcfd8d6e403071ae01374e2b66
ENV CLANG_SHA c1801030a9e8b29bcdde64113079495cd32d1537

# Git doesn't support direct checkout of a commit:
RUN mkdir llvm && cd llvm && git init && \
    git remote add origin https://github.com/llvm-mirror/llvm && \
    git fetch --depth 1 origin ${LLVM_SHA} && git checkout FETCH_HEAD
  # Also could use main git repo: https://git.llvm.org/git/llvm.git/

RUN mkdir clang && cd clang && git init && \
    git remote add origin https://github.com/llvm-mirror/clang && \
    git fetch --depth 1 origin ${CLANG_SHA} && git checkout FETCH_HEAD

RUN wget -nv http://www.openfst.org/twiki/pub/FST/FstDownload/openfst-1.6.9.tar.gz -O /tmp/openfst.tgz && \
    tar xf /tmp/openfst.tgz && \
    mv openfst-1.6.9 openfst

RUN cd openfst && ./configure && make -j && make install

# FIXME: push this back up in the Makefile:
RUN mv /build/clang /build/llvm/tools

ADD tree-fuser /build/llvm/tools/clang/tools/

RUN echo 'add_clang_subdirectory(tree-fuser)' >> /build/llvm/tools/clang/tools/CMakeLists.txt

# FIXME: move earlier and coalesce:
RUN apt-get install -y cmake ninja-build
RUN apt-get install -y python
# RUN apt-get install -y z3
# RUN apt-get install -y build-essential libc++-dev binutils

RUN mkdir /build/llvm-bld && cd llvm-bld && \
    cmake -G Ninja ../llvm && \
    ninja tree-fuser


# sha256=de5959c0c7decd920068aa4f9405769842b955719d857fd5d858fcacf0998bda
