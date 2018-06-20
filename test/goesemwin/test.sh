#!/bin/bash

set -ex

test_mode() {
    dir="./gen-$1"
    rm -rf "$dir"
    mkdir -p "$dir"
    goesemwin --out "$dir" --mode $1 ./packets/*.raw | grep -v Writing
}

test_mode raw
test_mode qbt
test_mode emwin
