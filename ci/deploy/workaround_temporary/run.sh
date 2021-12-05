#!/usr/bin/env bash

# FIXME(#164): remove this script and add simpler runner to main build system

set -e
set -x

yarn install
LD_LIBRARY_PATH="$PWD" node dist_back/index.js
