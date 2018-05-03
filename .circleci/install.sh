#!/bin/bash

set -ex

apt-get update

apt-get install -y \
        build-essential \
        cmake \
        git-core \
        libopencv-dev \
        zlib1g-dev
