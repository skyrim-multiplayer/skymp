FROM ubuntu:21.10

WORKDIR /usr/src/skymp

# Prevent apt-get from asking us about timezone
# London is UTC+0:00
ENV TZ=Europe/London
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# Install system dependencies via apt-get
# python2 and libicu-dev are required to build Chakracore
# pkg-config is required for zlib
RUN apt-get update && apt-get install -y \
  python2 \
  libicu-dev \
  clang \
  git \
  cmake \
  ninja-build \
  curl \
  unzip \
  tar \
  perl \
  make \
  zip \
  pkg-config \
  upx-ucl \
  && rm -rf /var/lib/apt/lists/*

ENV CC=/usr/bin/clang-12
ENV CPP=/usr/bin/clang-cpp-12
ENV CXX=/usr/bin/clang++-12
ENV LD=/usr/bin/ld.lld-12

# Bootstrap vcpkg
# (vcpkg/refs/heads/master contains vcpkg version)
COPY .git/modules/vcpkg/refs/heads/master ./master
RUN git clone https://github.com/skyrim-multiplayer/vcpkg.git \ 
  && cd vcpkg \
  && git checkout $(cat /usr/src/skymp/master) \
  && rm /usr/src/skymp/master \
  && chmod 777 ./bootstrap-vcpkg.sh \
  && ./bootstrap-vcpkg.sh -useSystemBinaries -disableMetrics

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

# Install NodeJS via Node Version Manager
ENV NODE_VERSION=14.16.0
RUN curl -o- https://raw.githubusercontent.com/creationix/nvm/v0.34.0/install.sh | bash
ENV NVM_DIR=/root/.nvm
RUN . "$NVM_DIR/nvm.sh" && nvm install ${NODE_VERSION}
RUN . "$NVM_DIR/nvm.sh" && nvm use v${NODE_VERSION}
RUN . "$NVM_DIR/nvm.sh" && nvm alias default v${NODE_VERSION}
ENV PATH="/root/.nvm/versions/node/v${NODE_VERSION}/bin/:${PATH}"

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
  && npm cache verify \
  && mkdir -p build \
  && cd build \ 
  && cmake .. -DCMAKE_BUILD_TYPE=Release -DUNIT_DATA_DIR=/usr/src/skymp/build/dist/server/data \
  && cmake --build . --config Release
