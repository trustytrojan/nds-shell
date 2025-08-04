#!/bin/sh
find src include -name '*.*pp' | xargs clang-format -i