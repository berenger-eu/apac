FROM debian:12.5

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    cmake \
    git \
    g++ \
    zlib1g-dev \
    libzstd-dev \
    wget \
    software-properties-common \
    gnupg \
    lsb-release \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

RUN wget https://apt.llvm.org/llvm.sh && \
    chmod +x llvm.sh && \
    ./llvm.sh 18 all && \
    rm llvm.sh

RUN apt-get update && apt-get install -y \
    clang-format \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
