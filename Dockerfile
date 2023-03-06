FROM ubuntu:focal

WORKDIR /usr/src/skymp

# Prevent apt-get from asking us about timezone
ENV TZ=Etc/GMT
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN \
  apt-get update && apt-get install -y curl \
  && curl -fsSL https://deb.nodesource.com/setup_18.x | bash - \
  && apt-get update \
  && apt-get install -y nodejs yarn gdb \
  && rm -rf /var/lib/apt/lists/*

# Install system dependencies via apt-get
# python2 and libicu-dev are required to build Chakracore
# pkg-config is required for zlib
# TODO: update clang
RUN \
  curl -fsSL https://dl.yarnpkg.com/debian/pubkey.gpg | gpg --dearmor > /usr/share/keyrings/yarnkey.gpg \
  && echo "deb [signed-by=/usr/share/keyrings/yarnkey.gpg] https://dl.yarnpkg.com/debian stable main" > /etc/apt/sources.list.d/yarn.list \
  && curl -fsSL https://apt.kitware.com/keys/kitware-archive-latest.asc | gpg --dearmor - > /usr/share/keyrings/kitware-archive-keyring.gpg \
  && echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ focal main' > /etc/apt/sources.list.d/kitware.list \
  && apt-get update \
  && apt-get install -y \
    nodejs \
    yarn \
    python2 \
    libicu-dev \
    git \
    cmake \
    curl \
    unzip \
    tar \
    make \
    zip \
    pkg-config \
    cmake \
    clang-12 \
    ninja-build \
  && rm -rf /var/lib/apt/lists/*

ENV CC=/usr/bin/clang-12
ENV CPP=/usr/bin/clang-cpp-12
ENV CXX=/usr/bin/clang++-12
ENV LD=/usr/bin/ld.lld-12

# Bootstrap vcpkg
# (vcpkg/refs/heads/master contains vcpkg version)
COPY ./.git/modules/vcpkg/HEAD ./master
RUN git clone https://github.com/microsoft/vcpkg.git \ 
  && cd vcpkg \
  && git checkout $(cat /usr/src/skymp/master) \
  && rm /usr/src/skymp/master \
  && chmod 777 ./bootstrap-vcpkg.sh \
  && ./bootstrap-vcpkg.sh -useSystemBinaries

# Currently needed for Chakracore
# TODO: Update to latest vcpkg where our Chakracore port fix has been shipped
COPY ./overlay_ports ./overlay_ports

# https://github.com/chakra-core/ChakraCore/blob/6800c46e2bcb5eafd81f19716a4f9f09774f134b/bin/ch/CMakeLists.txt#L3
RUN ln -s /usr/bin/python2.7 /usr/bin/python

# Install heavy ports first. Currently only Chakracore
RUN vcpkg/vcpkg \
  --feature-flags=binarycaching \
  --triplet x64-linux \
  --overlay-ports=./overlay_ports \
  install chakracore \
  && rm -r vcpkg/buildtrees \
  && rm -r vcpkg/packages \
  && rm -r vcpkg/downloads

# Install ports specified in vcpkg.json
COPY ./vcpkg.json ./
RUN vcpkg/vcpkg --feature-flags=binarycaching,manifests install --triplet x64-linux --overlay-ports=./overlay_ports \
  && rm -r vcpkg/buildtrees \
  && rm -r vcpkg/packages \
  && rm -r vcpkg/downloads

# Currently, we have only one overlay triplet and it is for Windows
# But missing directory would break CMake build
COPY ./overlay_triplets ./overlay_triplets

RUN mkdir -p build/dist/server/data \
  && cd build/dist/server/data \
  && curl -LJO https://gitlab.com/pospelov/se-data/-/raw/main/Skyrim.esm \
  && curl -LJO https://gitlab.com/pospelov/se-data/-/raw/main/Update.esm \
  && curl -LJO https://gitlab.com/pospelov/se-data/-/raw/main/Dawnguard.esm \
  && curl -LJO https://gitlab.com/pospelov/se-data/-/raw/main/HearthFires.esm \
  && curl -LJO https://gitlab.com/pospelov/se-data/-/raw/main/Dragonborn.esm

# Build the project and install missing vcpkg dependencies if any
COPY . .
RUN rm -rf ./skymp5-server/cmake-js-fetch-build || true \
  && mkdir -p build \
  && cd build \ 
  && cmake .. -DCMAKE_BUILD_TYPE=Release -DUNIT_DATA_DIR=/usr/src/skymp/build/dist/server/data \
  && cmake --build . --config Release

WORKDIR /usr/src/skymp/build/dist/server
CMD ["node", "./dist_back/skymp5-server.js"]
