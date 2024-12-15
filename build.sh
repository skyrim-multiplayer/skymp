#!/bin/sh

eecho() {
  echo "$@" >&2
}

if [ "`basename "$PWD"`" = "build" ]; then
  cd ..
fi

if [ ! -e build.sh ]; then
  eecho "This script should be run from either source root or build directory."
  exit 1
fi

export VCPKG_DISABLE_METRICS=1

export CC=/usr/bin/clang-15
export CPP=/usr/bin/clang-cpp-15
export CXX=/usr/bin/clang++-15
export LD=/usr/bin/ld.lld-15

export CMAKE_C_COMPILER="$CC"
export CMAKE_CXX_COMPILER="$CXX"
export CMAKE_MAKE_PROGRAM="ninja"

# Some build dependencies require some strange stuff.
# Some other deps won't work if we can't call clang without exact version.
# It's better to make compatibility aliases than breaking main system's root.
export SKYMP_COMPAT_BIN="$PWD/build/skymp_compat/bin"
export PATH="$SKYMP_COMPAT_BIN:$PATH"

if [ ! -d build ]; then
  mkdir -v build
fi

if [ ! -d "$SKYMP_COMPAT_BIN" ]; then
  mkdir -pv "$SKYMP_COMPAT_BIN"
  ln -s "$CC" "$SKYMP_COMPAT_BIN/clang"
  ln -s "$CXX" "$SKYMP_COMPAT_BIN/clang++"

  echo "Set up compatibility path for build."
fi

# TODO reverse the order or use [[ ]] ?
if [ "$1" = "--configure" ]; then
  shift && \
    cd build && \
    exec cmake -G "Ninja" .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON "$@"
elif [ "$1" = "--build" ]; then
  shift && \
    cd build && \
    exec cmake --build . "$@"
elif [ "$1" = "--clean" ]; then
  eecho "NOTE: --clean was deprecated, please use --clean-cpp or --clean-node"
  eecho "Proceeding with cleaning cpp build data"
  exec rm -rf build/
elif [ "$1" = "--clean-cpp" ]; then
  exec rm -rf build/
elif [ "$1" = "--clean-node" ]; then
  echo not implemented yet, TODO delete node_modules dirs
else
  eecho "Usage:"
  eecho "  ./build.sh --configure <cmake args...>"
  eecho "OR"
  eecho "  ./build.sh --build <cmake args...>"
  eecho "OR"
  eecho "  ./build.sh --clean-<cpp|node>"
fi
