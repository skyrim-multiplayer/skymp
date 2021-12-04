# This is the base image for building SkyMP source.
# It contains everything that should be installed on the system.
FROM ubuntu:focal AS skymp-base

# Prevent apt-get from asking us about timezone
# London is UTC+0:00
ENV TZ=Europe/London
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN \
  apt-get update && apt-get install -y curl \
  && curl -fsSL https://deb.nodesource.com/setup_14.x | bash - \
  && curl -fsSL https://dl.yarnpkg.com/debian/pubkey.gpg | gpg --dearmor > /usr/share/keyrings/yarnkey.gpg \
  && echo "deb [signed-by=/usr/share/keyrings/yarnkey.gpg] https://dl.yarnpkg.com/debian stable main" > /etc/apt/sources.list.d/yarn.list \
  && curl -fsSL https://apt.kitware.com/keys/kitware-archive-latest.asc | gpg --dearmor - > /usr/share/keyrings/kitware-archive-keyring.gpg \
  && echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ focal main' > /etc/apt/sources.list.d/kitware.list \
  && apt-get update \
  && apt-get install -y \
    nodejs \
    yarn \
    # dsadsa
    python2 \
    libicu-dev \
    clang-12 \
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
    cmake=3.22.0-0kitware1ubuntu20.04.1 \
  && rm -rf /var/lib/apt/lists/*

RUN useradd -m skymp


# Intermediate image to build
# TODO: copy less stuff, use args to pass the desired vcpkg submodule revision
# TODO: build huge deps separately
FROM skymp-base AS skymp-vcpkg-deps-builder

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
