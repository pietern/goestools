#!/bin/bash
#
# Setup environment for cross-compilation for Raspbian.
#

set -e

target_dir="xcompile/raspbian"
mkdir -p "${target_dir}"
cp -f "$(dirname "$0")/files/raspberrypi.cmake" "${target_dir}"

# Checkout tools repository if needed
if [ ! -d "${target_dir}/tools" ]; then
    git clone https://github.com/raspberrypi/tools.git "${target_dir}/tools"
fi

urls() {
    (
        scripts/list_raspbian_urls.py \
            librtlsdr-dev \
            libairspy-dev \
            libusb-1.0-0-dev \
            libudev1
    ) | sort | uniq
}

tmp=$(mktemp -d)
trap "rm -rf $tmp" EXIT

# Download packages of interest
for url in $(urls); do
    echo "Downloading ${url}..."
    ( cd "$tmp" && curl -LOs "${url}" )
done

# Extract into sysroot
for deb in "$tmp"/*.deb; do
    echo "Extracting $(basename "${deb}")..."
    dpkg-deb -x "${deb}" "${target_dir}/tools/arm-bcm2708/arm-linux-gnueabihf/arm-linux-gnueabihf/sysroot"
done
