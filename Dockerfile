FROM ubuntu:21.10

WORKDIR /usr/src/skymp

# Prevent apt-get from asking us about timezone
# London is UTC+0:00
ENV TZ=Europe/London
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# Install system dependencies via apt-get
# Python 2.7 is required to build Chakracore
RUN apt-get update && apt-get install -y \
  python2 \
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

# Install heavy ports first. Currently only Chakracore
RUN vcpkg/vcpkg \
  --feature-flags=binarycaching \
  --triplet x64-linux \
  --overlay-ports=./overlay_ports \
  install chakracore \
  && rm -r vcpkg/buildtrees \
  && rm -r vcpkg/packages \
  && rm -r vcpkg/downloads

COPY ./vcpkg.json ./

RUN vcpkg/vcpkg --feature-flags=binarycaching,manifests install --triplet x64-linux --overlay-ports=./overlay_ports \
  && rm -r vcpkg/buildtrees \
  && rm -r vcpkg/packages \
  && rm -r vcpkg/downloads

# Build the project
COPY ./CMakeLists.txt ./.clang-format ./
COPY ./cmake ./cmake
COPY ./chakra-wrapper ./chakra-wrapper
COPY ./skyrim-platform ./skyrim-platform
COPY ./skymp5-client ./skymp5-client
COPY ./skymp5-front ./skymp5-front
COPY ./skymp5-functions-lib ./skymp5-functions-lib
COPY ./skymp5-scripts ./skymp5-scripts
COPY ./client-deps ./client-deps
COPY ./skymp5-server ./skymp5-server
RUN mkdir build \
  && cd build \ 
  && cmake .. \
  && cmake --build . --config Release
