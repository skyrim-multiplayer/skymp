#!/bin/bash
set -e
yarn --cwd skyrim-platform/tools/const_enum_extractor install --frozen-lockfile
node skyrim-platform/tools/const_enum_extractor/index.js
