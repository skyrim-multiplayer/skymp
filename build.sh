#!/bin/sh

[ -d build ] || mkdir -v build

export VCPKG_DISABLE_METRICS=1

export CC=/usr/bin/clang-12
export CPP=/usr/bin/clang-cpp-12
export CXX=/usr/bin/clang++-12
export LD=/usr/bin/ld.lld-12

export CMAKE_C_COMPILER="$CC"
export CMAKE_CXX_COMPILER="$CXX"
export CMAKE_MAKE_PROGRAM="ninja"

export SKYMP_CMAKE_VARS="-DCMAKE_C_COMPILER=$CC -DCMAKE_CXX_COMPILER=$CXX -DCMAKE_MAKE_PROGRAM=ninja -DSKYRIM_DIR=~/.steam/debian-installation/steamapps/common/Skyrim\ Special\ Edition"

export SKYMP_COMPAT_BIN="$PWD/build/skymp_compat/bin"
mkdir -p "$SKYMP_COMPAT_BIN"
ln -s "`which python2`" "$SKYMP_COMPAT_BIN/python"
ln -s "$CC" "$SKYMP_COMPAT_BIN/clang" 
ln -s "$CXX" "$SKYMP_COMPAT_BIN/clang++"
export PATH="$SKYMP_COMPAT_BIN:$PATH"

echo Variables set. Should be sourced.
