#!/usr/bin/env bash

set -e

if [[ "$1" == "--get-image-uri" ]]; then
  . misc/github_env_linux && echo "$SKYMP_VCPKG_DEPS_IMAGE"
  exit
fi

if [[ "$1" == "--ensure-deps-noninteractive" ]]; then
  if [[ -z "$CI" ]]; then
    echo "This isn't CI environment! Are you sure you didn't want to use just --ensure-deps?"
    exit 1
  fi
  echo Nothing to prepare, vcpkg-deps image already contains all we need
  exit
fi

echo unexpected args
exit 1
