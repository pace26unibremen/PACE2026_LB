FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    cmake ninja-build clang git \
    zlib1g-dev libgmp-dev libreadline-dev \
    gfortran \
    libpcre2-dev \
    && rm -rf /var/lib/apt/lists/*

ADD scipoptsuite-10.0.2-glibc2_34-amd64.tgz /tmp/scip/

RUN SCIP_DIR=$(ls /tmp/scip/) \
    && cp -r /tmp/scip/$SCIP_DIR/include/* /usr/local/include/ \
    && cp -r /tmp/scip/$SCIP_DIR/bin/* /usr/local/bin/ \
    && mkdir -p /usr/local/lib64 \
    && find /tmp/scip/$SCIP_DIR/lib64/ -maxdepth 1 -not -type d \
       ! -name "libstdc++*" \
       ! -name "libgcc*" \
       ! -name "libgfortran*" \
       ! -name "libgomp*" \
       ! -name "libquadmath*" \
       -exec cp {} /usr/local/lib64/ \; \
    && mkdir -p /usr/local/lib/cmake \
    && cp -r /tmp/scip/$SCIP_DIR/lib64/cmake/* /usr/local/lib/cmake/ \
    && ldconfig \
    && rm -rf /tmp/scip