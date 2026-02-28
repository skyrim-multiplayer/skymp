#!/usr/bin/env bash

# Scroll down for the list of packages

set -e

if [[ "$1" == "--get-image-uri" ]]; then
  # This is the image that we'll use in our CI for testing build for this distro
  echo 'archlinux:base-devel'
  exit
fi

if [[ "$1" == "--ensure-deps-noninteractive" ]]; then
  if [[ -z "$CI" ]]; then
    echo "This isn't CI environment! Are you sure you didn't want to use just --ensure-deps?"
    exit 1
  fi
  set -x
  PACMAN_ARGS="--noconfirm"
  pacman -Syu $PACMAN_ARGS sudo --needed
  useradd -m skymp -u $CREATE_UID
  chown -R skymp:skymp /src

  cat /etc/passwd

  DO_ENSURE_DEPS=1
fi

if [[ "$1" == "--ensure-deps" ]]; then
  DO_ENSURE_DEPS=1
  PACMAN_ARGS="--confirm"
fi

if [[ -z "$DO_ENSURE_DEPS" ]]; then
  echo "idk what to do"
  exit 1
fi

packages=''

addpackage() {
  packages+="$1 "
}

# These are some basic packages that are essential for C++ build config that we're using
addpackage base-devel
addpackage cmake
addpackage clang
addpackage ninja  # would likely build with the regular make, but I haven't tried
# gdb is also recommended for debugging but isn't required

# These are needed for some parts of the client and server, as well as some build scripts
addpackage nodejs-lts-jod  # 22
# alternatively, nodejs for 23 (as of 20250118), nodejs-lts-iron for 20 or nodejs-lts-hydrogen for 18
# see https://archlinux.org/packages/?sort=&q=nodejs&maintainer=&flagged=
addpackage yarn

# Some packages that you likely already have, but we'll just make sure
addpackage git
addpackage zip
addpackage unzip
addpackage tar
addpackage curl  # required by vcpkg
addpackage pkgconf  # required by Catch2 (C++ testing framework)
addpackage linux-headers  # required by OpenSSL(?)
addpackage automake
addpackage libtool

echo Will now run the installation command, please check it and confirm
set -x  # this will print the list that we're going to install
sudo pacman -S --needed $PACMAN_ARGS $packages
