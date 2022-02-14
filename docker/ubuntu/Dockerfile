ARG version
FROM ubuntu:${version}

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

RUN apt-get update && apt-get install -y \
  build-essential \
  cmake \
  git-core \
  libairspy-dev \
  libopencv-dev \
  libproj-dev \
  librtlsdr-dev \
  zlib1g-dev \
  && rm -rf /var/lib/apt/lists/*

COPY ./run_cmake.sh /
COPY ./run_compile.sh /
