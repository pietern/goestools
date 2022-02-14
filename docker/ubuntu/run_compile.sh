#!/bin/sh

set -e

cd /tmp/goestools
make -j $(nproc)
