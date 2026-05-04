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
    clang-18 \
    libclang-18-dev \
    llvm-18-dev \
    clang-format-18 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /apac
COPY . .

RUN mkdir -p build && \
    cd build && \
    cmake -DLLVM_DIR=/usr/lib/llvm-18/cmake .. && \
    make -j$(nproc) && \
    strip apac taskGraph declarationSplitter duplicateFunctions \
          conditionUnstack multipleDeclSplitter constify gotoRet \
          unstack stackheap mainParallel apacDepth

ENV PATH="/apac/build:${PATH}"

WORKDIR /workspace
