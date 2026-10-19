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

BUILD_GENERATOR="Ninja"
CMAKE_MAKE_PROGRAM_PATH=""

try_clang() {
  suffix="$1"
  if which clang++$suffix > /dev/null 2>&1; then
    export CC="/usr/bin/clang$suffix"
    export CPP="/usr/bin/clang-cpp$suffix"
    export CXX="/usr/bin/clang++$suffix"
    echo "Using $CXX"
    return 0
  fi
  return 1
}

USE_SYSTEM_COMPILER=0
if [ "$1" = "--system-compiler" ]; then
  USE_SYSTEM_COMPILER=1
  shift
fi

if [ "$USE_SYSTEM_COMPILER" = "1" ]; then
  echo "Using system compiler: $(which cc) / $(which c++)"
else
  try_clang -20 || try_clang -15 || try_clang "" || (eecho "Could not find clang!" && exit 1)
fi

export CMAKE_C_COMPILER="$CC"
export CMAKE_CXX_COMPILER="$CXX"

ninja_path="$(command -v ninja || true)"
if [ -n "$ninja_path" ]; then
  BUILD_GENERATOR="Ninja"
  CMAKE_MAKE_PROGRAM_PATH="$ninja_path"
elif command -v make >/dev/null 2>&1; then
  BUILD_GENERATOR="Unix Makefiles"
  CMAKE_MAKE_PROGRAM_PATH="$(command -v make)"
  eecho "ninja not found, falling back to Unix Makefiles (${CMAKE_MAKE_PROGRAM_PATH})"
else
  eecho "Neither ninja nor make was found in PATH."
  exit 1
fi

export CMAKE_MAKE_PROGRAM="$CMAKE_MAKE_PROGRAM_PATH"
export CMAKE_GENERATOR="$BUILD_GENERATOR"

if [ ! -d build ]; then
  mkdir -v build
fi

# TODO(#2280): reverse the order or use [[ ]] ?
if [ "$1" = "--configure" ]; then
  shift && \
    cd build && \
    exec cmake -G "$BUILD_GENERATOR" .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON "$@"
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
