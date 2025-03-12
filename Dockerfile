FROM node:22.14.0 AS builder

ENV CI=true
ENV TZ=Etc/GMT

RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# TODO: update clang
RUN \
  curl -fsSL https://dl.yarnpkg.com/debian/pubkey.gpg | gpg --dearmor > /usr/share/keyrings/yarnkey.gpg \
  && echo "deb [signed-by=/usr/share/keyrings/yarnkey.gpg] https://dl.yarnpkg.com/debian stable main" > /etc/apt/sources.list.d/yarn.list \
  && curl -fsSL https://apt.kitware.com/keys/kitware-archive-latest.asc | gpg --dearmor - > /usr/share/keyrings/kitware-archive-keyring.gpg \
  && echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ jammy main' > /etc/apt/sources.list.d/kitware.list \
  && apt-get update \
  && apt-get install -y \
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
    clang-15 \
    clang-format-15 \
    ninja-build \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /src

# TODO: use build.sh --configure with dummy cmakelists for early ports install

#COPY ./vcpkg ./vcpkg
#COPY ./vcpkg.json ./vcpkg.json

#RUN cd vcpkg \
#  && chmod +x ./bootstrap-vcpkg.sh \
#  && ./bootstrap-vcpkg.sh \
#  && mkdir -p /src/build/vcpkg_installed \
#  && ./vcpkg install --triplet x64-linux --x-install-root=/src/build/vcpkg_installed --x-manifest-root=/src

COPY . .

RUN ./build.sh --configure \
    -DBUILD_UNIT_TESTS=OFF \
    -DBUILD_GAMEMODE=OFF \
    -DBUILD_CLIENT=OFF \
    -DBUILD_FRONT=OFF \
    -DBUILD_SKYRIM_PLATFORM=OFF \
    -DOFFLINE_MODE=OFF \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo
RUN ./build.sh --build

# --- Second Stage ---
FROM node:22.14.0 AS runner

ENV CI=true
ENV TZ=Etc/GMT

RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

WORKDIR /src

COPY --from=builder /src/build/dist/server /src

CMD ["node", "dist_back"]
