FROM --platform=linux/amd64 mcr.microsoft.com/devcontainers/javascript-node:22-bookworm

# Tip for maintainers: Updating Vcpkg Binary Cache
# 1. Run CMake
# 2. scp ~/.cache/vcpkg/archives/*/*.zip codespaces-vcpkg-binary-cache@storage.bunnycdn.com:/

ENV VCPKG_BINARY_SOURCES="http,https://codespaces-vcpkg-binary-cache.b-cdn.net/{sha}.zip,read"

RUN \
  curl -fsSL https://dl.yarnpkg.com/debian/pubkey.gpg | gpg --dearmor > /usr/share/keyrings/yarnkey.gpg \
  && echo "deb [signed-by=/usr/share/keyrings/yarnkey.gpg] https://dl.yarnpkg.com/debian stable main" > /etc/apt/sources.list.d/yarn.list \
  && curl -fsSL https://apt.kitware.com/keys/kitware-archive-latest.asc | gpg --dearmor - > /usr/share/keyrings/kitware-archive-keyring.gpg \
  && echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ jammy main' > /etc/apt/sources.list.d/kitware.list \
  && apt-get update \
  && apt-get install -y \
    cmake \
    clang-15=1:15.0.6-4+b1 \
    clang-format-15=1:15.0.6-4+b1 \
    ninja-build=1.11.1-2~deb12u1 \
    yarn \
    libicu-dev \
    git \
    curl \
    unzip \
    tar \
    make \
    zip \
    pkg-config \
    flex \
  && rm -rf /var/lib/apt/lists/* \
  && mv /usr/bin/clang-format-15 /usr/bin/clang-format

  # P.S. flex is for libpq
