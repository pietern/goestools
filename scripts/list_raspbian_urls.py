#!/usr/bin/env python
#
# Find paths of packages and their dependencies
# by parsing Packages file from Raspbian repository.
#

import argparse
import re
import sys
import urllib


parser = argparse.ArgumentParser()
parser.add_argument('-v', '--verbose', action='store_true')
parser.add_argument('packages', metavar='PACKAGE', type=str, nargs='+')
args = parser.parse_args()


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
queue = args.packages
skip = set()
while len(queue) > 0:
    package = queue.pop(0)
    if not package in packages:
        if args.verbose:
            sys.stderr.write("Package doesn't exist: {}\n".format(package))
        continue

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
