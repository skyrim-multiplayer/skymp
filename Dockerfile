#
# docker build -t skymp . --platform linux/amd64
#

#
# docker run --rm --name myserver \
# -p 7777:7777/udp -p 3000:3000 -p 8080:8080 \
# -v $(pwd)/build/dist/server/gamemode.js:/usr/src/skymp/build/dist/server/gamemode.js \
# -v "<your_data_dir>:/skyrim_data_dir" \
# skymp

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

COPY ./skymp5-client ./skymp5-client
COPY ./skymp5-functions-lib ./skymp5-functions-lib
COPY ./skymp5-server ./skymp5-server
COPY ./skymp5-front ./skymp5-front
COPY ./skymp5-scripts ./skymp5-scripts
COPY ./skyrim-platform ./skyrim-platform
COPY ./unit ./unit
COPY ./viet ./viet
COPY ./client-deps ./client-deps
COPY CMakeLists.txt ./
COPY .clang-format ./
COPY cmake ./cmake
COPY ./1js ./1js

RUN mkdir -p build \
  && cd build \
  && cmake .. -DCMAKE_BUILD_TYPE=Release \
  && cmake --build . --config Release

WORKDIR /usr/src/skymp/build/dist/server

# TODO: make volume for this file
COPY misc/docker-server-settings.json ./server-settings.json

CMD ["node", "./dist_back/skymp5-server.js"]
