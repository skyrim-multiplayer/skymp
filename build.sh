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
  shift && cd build || exit 1
  cmake -G "Ninja" .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON "$@"
  ret=$?
  if [ $ret -ne 0 ]; then
    echo "Configure failed. Printing logs..."
    if [ -f "vcpkg-manifest-install.log" ]; then
      echo "====== vcpkg-manifest-install.log ======"
      cat "vcpkg-manifest-install.log"
    fi
    if [ -d "../vcpkg/buildtrees" ]; then
      find "../vcpkg/buildtrees" -name "*.log" -exec echo "====== {} ======" \; -exec cat {} \;
    fi
    exit $ret
  fi
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
