#syntax=docker/dockerfile:1

FROM quay.io/centos/centos:stream9 AS builder

RUN dnf install -y \
      git \
      cmake \
      gcc-c++ \
      elfutils-libelf-devel \
      elfutils-devel \
      autoconf \
      automake \
      libtool \
    && dnf clean all

COPY . /tmp/uwpmp

RUN cd /tmp/uwpmp \
    && rm -rf build \
    && mkdir build \
    && cd build \
    && cmake .. \
    && make -j $(nproc)

FROM quay.io/centos/centos:stream9

RUN dnf install -y \
      elfutils-libs \
      procps-ng \
    && dnf clean all \
    && rm -rf /var/cache/dnf

COPY --from=builder /tmp/uwpmp/build/unwindpmp /usr/local/bin/unwindpmp
