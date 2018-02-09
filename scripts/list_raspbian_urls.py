#!/usr/bin/env python
#
# Find paths of packages and their dependencies
# by parsing Packages file from Raspbian repository.
#

import re
import sys
import urllib


# Load list of packages
mirror = "http://mirrordirector.raspbian.org/raspbian"
response = urllib.urlopen(mirror + "/dists/stretch/main/binary-armhf/Packages")
body = response.read()


# Parse packages in map of dicts (keyed by package name)
packages = dict()
chunk = re.compile("^Package: (.*?)$.*?\n$", re.M | re.S)
pair = re.compile("^(\w+): (.*)$", re.M)
for m1 in chunk.finditer(body):
    out = dict()
    for m2 in pair.finditer(m1.group(0)):
        out[m2.group(1)] = m2.group(2)
    packages[m1.group(1)] = out


# List recursive dependencies
queue = sys.argv[1:]
skip = set()
while len(queue) > 0:
    package = queue.pop(0)
    if not package in packages:
        sys.stderr.write("Package doesn't exist: {}\n".format(package))
        sys.exit(1)

    if package in skip:
        continue

    skip.add(package)
    kv = packages[package]
    print(mirror + "/" + kv['Filename'])

    depends = kv.get('Depends', '')
    for spec in depends.split(', '):
        if len(spec) == 0:
            continue
        match = re.match("^([^\s]+)", spec)
        queue.append(match.group(1))
