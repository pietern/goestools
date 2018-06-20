#!/bin/bash

set -ex

mkdir -p packets
curl https://s3.amazonaws.com/goestools/test/goesemwin-goes15-packets-1h.tgz | tar -C ./packets -zxf -
