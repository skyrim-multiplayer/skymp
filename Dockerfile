FROM ubuntu:18.04

RUN apt-get update && apt-get install -y \
  clang-12 \
  git \
  && rm -rf /var/lib/apt/lists/*

# Install vcpkg and ports
# (vcpkg/refs/heads/master contains vcpkg version)
COPY .git/modules/vcpkg/refs/heads/master \
  ./vcpkg.json \
  ./
COPY ./overlay_ports ./overlay_ports
RUN git clone https://github.com/skyrim-multiplayer/vcpkg.git \ 
  && cd vcpkg \
  && git checkout $(cat master) \
  && chmod 777 ./bootstrap-vcpkg.sh \
  && ./bootstrap-vcpkg.sh -useSystemBinaries -disableMetrics \
  && cd .. \
  && vcpkg/vcpkg --feature-flags=binarycaching,manifests install --triplet x64-linux --overlay-ports=/overlay_ports \
  && rm -r vcpkg/buildtrees \
  && rm -r vcpkg/packages \
  && rm -r vcpkg/downloads

# Build the project
COPY ./CMakeLists.txt ./.clang-format ./
COPY ./cmake ./cmake
COPY ./scamp_native ./scamp_native
RUN mkdir build \
  && cd build \ 
  && cmake .. \
  && cmake --build . --config Release
