#!/bin/bash
#
# Setup environment for cross-compilation for Raspbian.
#

set -e

target_dir="xcompile/raspbian"
mkdir -p "${target_dir}"
cp -f "$(dirname "$0")/files/raspberrypi.cmake" "${target_dir}"

install_if_needed() {
    if ! dpkg -l "$1" | grep -q '^ii'; then
        sudo apt-get install -y "$1"
    fi
}

# These resolve to 5.5.0-12ubuntu1cross1 on Ubuntu 18.04
install_if_needed gcc-5-arm-linux-gnueabihf
install_if_needed g++-5-arm-linux-gnueabihf

urls() {
    scripts/list_raspbian_urls.py \
        librtlsdr-dev \
        libairspy-dev \
        libusb-1.0-0-dev \
        libudev1 \
        zlib1g-dev \
        libopencv-dev \
        libopencv-highgui-dev \
        libproj-dev \
        gcc-5 \
        g++-5
}

tmp="$target_dir/tmp"
mkdir -p "$tmp"

# Download and extract packages of interest
for url in $(urls); do
    deb=$(basename "${url}")
    if [ ! -f "${tmp}/${deb}" ]; then
        echo "Downloading ${url}..."
        ( cd "$tmp" && curl -LOs "${url}" )
    fi
    echo "Extracting ${deb}..."
    dpkg-deb -x "${tmp}/${deb}" "${target_dir}/sysroot"
done
