# syntax=docker/dockerfile:1

FROM ubuntu:oracular

RUN --mount=type=bind,source=./install-dependencies.sh,target=/install-dependencies.sh \
    apt-get update && apt-get install -y \
    curl \
    git  \
    gnupg \
    && echo "deb http://apt.llvm.org/oracular/ llvm-toolchain-oracular-18 main" \
    >> /etc/apt/sources.list.d/llvm.list \
    && echo "deb http://apt.llvm.org/oracular/ llvm-toolchain-oracular-19 main" \
    >> /etc/apt/sources.list.d/llvm.list \
    && curl -sSL https://apt.llvm.org/llvm-snapshot.gpg.key -o /etc/apt/trusted.gpg.d/apt.llvm.org.asc \
    && apt-get update && apt-get install -y \
    build-essential \
    clang-18 \
    clang-19 \
    clang-tools-19 \
    gcc-13 \
    g++-13 \
    gcc-14 \
    g++-14 \
    pandoc \
    cmake \
    wget \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 13 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 13 \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-14 14 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-14 14 \
    && update-alternatives --install /usr/bin/clang clang /usr/bin/clang-18 18 \
    && update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-19 19 \
    && update-alternatives --install /usr/bin/clang clang /usr/bin/clang-19 19 \
    && update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-19 19

RUN wget https://github.com/c-ares/c-ares/releases/download/v1.34.4/c-ares-1.34.4.tar.gz \
    && tar -xvzf c-ares-1.34.4.tar.gz \
    && cd c-ares-1.34.4 \
    && ./configure \
    && make \
    && make install \
    && cd .. \
    && rm -rf c-ares-1.34.4.tar.gz c-ares-1.34.4

RUN --mount=type=bind,source=./install-dependencies.sh,target=/install-dependencies.sh \
    apt-get update \
    && bash ./install-dependencies.sh


RUN apt-get clean \
    && rm -rf /var/lib/apt/lists/*

CMD ["/bin/bash"]