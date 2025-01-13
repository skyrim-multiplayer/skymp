#!/usr/bin/env bash

set -e
set -x

LD_PRELOAD=/usr/lib/libasan.so ASAN_OPTIONS=verbosity=1:detect_leaks=0 node dist_back/skymp5-server.js
