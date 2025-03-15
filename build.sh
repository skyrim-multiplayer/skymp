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

if which clang++ > /dev/null 2>&1; then
  export CC=/usr/bin/clang
  export CPP=/usr/bin/clang-cpp
  export CXX=/usr/bin/clang++
else
  export CC=/usr/bin/clang-15
  export CPP=/usr/bin/clang-cpp-15
  export CXX=/usr/bin/clang++-15
fi

export CMAKE_C_COMPILER="$CC"
export CMAKE_CXX_COMPILER="$CXX"
export CMAKE_MAKE_PROGRAM="/usr/bin/ninja"

if [ ! -d build ]; then
  mkdir -v build
fi

# TODO(#2280): reverse the order or use [[ ]] ?
if [ "$1" = "--configure" ]; then
  echo build.sh: added WITH_ANTIGO
  shift && \
    cd build && \
    exec cmake -G "Ninja" .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DWITH_ANTIGO=1 "$@"
elif [ "$1" = "--build" ]; then
  shift && \
    cd build && \
    exec cmake --build . "$@"
elif [ "$1" = "--clean" ]; then
  eecho "NOTE: --clean was deprecated, please use one of the options listed in --help"
  eecho "Proceeding with cleaning cpp build data"
  exec rm -rf build/
elif [ "$1" = "--clean-cpp" ]; then
  exec rm -rf build/
elif [ "$1" = "--clean-node" ]; then
  find -name node_modules -type d -prune -print0 | xargs -0 echo rm -rf
  echo -n Ctrl-C to abort, Return/Enter to proceed
  read
  find -name node_modules -type d -prune -print0 | xargs --verbose -0 rm -rf
elif [ "$1" = "--clean-vcpkg" ]; then
  (cd vcpkg && git clean -xfd --dry-run)
  echo -n Ctrl-C to abort, Return/Enter to proceed
  read
  (cd vcpkg && git clean -xfd)
elif [ "$1" = "--print-env" ]; then
  env
else
  eecho "Usage:"
  eecho "  ./build.sh --configure <cmake args...>"
  eecho "  runs in build: cmake .. <some extra args> <your args>"
  eecho "OR"
  eecho "  ./build.sh --build <cmake args...>"
  eecho "  runs in build: cmake build . <your args>"
  eecho "  you can add args like --target=unit"
  eecho "OR"
  eecho "  ./build.sh --clean-<cpp|node|vcpkg>"
  eecho "OR"
  eecho "  ./build.sh --print-env"
  if [ "$1" != "--help" ]; then
    exit 1
  fi
fi
