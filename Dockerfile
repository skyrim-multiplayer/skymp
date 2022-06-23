# Image used as runtime base for a game server.
# Contains a minimal subset of stuff needed for running (and debugging, if needed) the server.
FROM ubuntu:focal AS skymp-runtime-base

# Prevent apt-get from asking us about timezone
# London is UTC+0:00
ENV TZ=Europe/London
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN \
  apt-get update && apt-get install -y curl \
  && curl -fsSL https://deb.nodesource.com/setup_16.x | bash - \
  && apt-get update \
  && apt-get install -y nodejs yarn gdb \
  && rm -rf /var/lib/apt/lists/*

RUN useradd -m skymp


# This is the base image for building SkyMP source.
# It contains everything that should be installed on the system.
FROM skymp-runtime-base AS skymp-build-base

# TODO: are perl, upx-ucl, ninja needed?
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
    ninja-build \
    curl \
    unzip \
    tar \
    perl \
    make \
    zip \
    pkg-config \
    upx-ucl \
    cmake=3.22.2-0kitware1ubuntu20.04.1 \
    clang-12 \
  && rm -rf /var/lib/apt/lists/*


# Intermediate image to build
# TODO: copy less stuff, use args to pass the desired vcpkg submodule revision
# TODO: build huge deps separately
FROM skymp-build-base AS skymp-vcpkg-deps-builder

COPY --chown=skymp:skymp . /src

USER skymp

RUN  cd /src \
  && git submodule update --init --recursive \
  && ./build.sh --configure


# Image that runs in CI. It contains vcpkg cache to speedup the build.
# Sadly, the builtin NuGet cache doesn't work on Linux, see:
# https://github.com/microsoft/vcpkg/issues/19038
FROM skymp-base AS skymp-vcpkg-deps

COPY --from=skymp-vcpkg-deps-builder --chown=skymp:skymp \
  /home/skymp/.cache/vcpkg /home/skymp/.cache/vcpkg
