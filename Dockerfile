FROM ubuntu:jammy AS builder

ENV CI=true
ENV TZ=Etc/GMT

RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN \
  apt-get update && apt-get install -y curl \
  && curl -fsSL https://deb.nodesource.com/setup_22.x | bash - \
  && apt-get update \
  && apt-get install -y nodejs yarn \
  && rm -rf /var/lib/apt/lists/*

# TODO: update clang
RUN \
  curl -fsSL https://dl.yarnpkg.com/debian/pubkey.gpg | gpg --dearmor > /usr/share/keyrings/yarnkey.gpg \
  && echo "deb [signed-by=/usr/share/keyrings/yarnkey.gpg] https://dl.yarnpkg.com/debian stable main" > /etc/apt/sources.list.d/yarn.list \
  && curl -fsSL https://apt.kitware.com/keys/kitware-archive-latest.asc | gpg --dearmor - > /usr/share/keyrings/kitware-archive-keyring.gpg \
  && echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ jammy main' > /etc/apt/sources.list.d/kitware.list \
  && apt-get update \
  && apt-get install -y \
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
    clang-15 \
    clang-format-15 \
    ninja-build \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /src

COPY ./vcpkg ./vcpkg

RUN cd vcpkg \
  && chmod +x ./bootstrap-vcpkg.sh \
  && ./bootstrap-vcpkg.sh \
  && cd .. \
  && ./vcpkg/vcpkg install

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
FROM ubuntu:jammy AS runner

ENV CI=true
ENV TZ=Etc/GMT

RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN \
  apt-get update && apt-get install -y curl \
  && curl -fsSL https://deb.nodesource.com/setup_22.x | bash - \
  && apt-get update \
  && apt-get install -y nodejs gdb \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /src

COPY --from=builder /src/build/dist/server /src

CMD ["node", "dist_back"]
