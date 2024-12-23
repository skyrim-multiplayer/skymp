#!/usr/bin/env bash

# Scroll down for the list of packages

set -e

if [[ "$1" == "--get-image-uri" ]]; then
  # This is the image that we'll use in our CI for testing build for this distro
  echo 'debian:bookworm'
  exit
fi

if [[ "$1" == "--ensure-deps-noninteractive" ]]; then
  if [[ -z "$CI" ]]; then
    echo "This isn't CI environment! Are you sure you didn't want to use just --ensure-deps?"
    exit 1
  fi
  set -x
  APT_ARGS="-y"
  apt-get update
  apt-get full-upgrade $APT_ARGS
  apt-get install $APT_ARGS sudo
  useradd -m skymp -u $CREATE_UID
  chown -R skymp:skymp /src

  cat /etc/passwd

  DO_ENSURE_DEPS=1
fi

if [[ "$1" == "--ensure-deps" ]]; then
  DO_ENSURE_DEPS=1
  APT_ARGS=""
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
addpackage build-essential
addpackage cmake
addpackage clang-15
addpackage clang-format-15  # you will need it to pass clang-format check
addpackage ninja-build  # would likely build with the regular make, but I haven't tried
# gdb is also recommended for debugging but isn't required

# These are needed for some parts of the client and server, as well as some build scripts
addpackage nodejs
addpackage npm  # npm should be installed in order for us to be able to install yarn
# yarn is also needed, but we'll install it with npm

# Some packages that you likely already have, but we'll just make sure
# They are needed by vcpkg and some of the used libraries
addpackage git
addpackage zip
addpackage unzip
addpackage tar
addpackage curl  # required by vcpkg
addpackage pkgconf  # required by Catch2 (C++ testing framework)
addpackage linux-libc-dev  # required by OpenSSL(?)

echo Will now run the installation command, please check it and confirm
set -x  # this will print the list that we're going to install
sudo apt-get install $APT_ARGS $packages
npm install -g yarn
