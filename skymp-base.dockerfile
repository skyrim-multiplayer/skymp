FROM node:14.13.1-alpine3.12
ENV VCPKG_FORCE_SYSTEM_BINARIES=1
RUN apk add --no-cache \
    gcc \
    musl-dev \
    g++ \
    cmake \
    gdb \
    git \
    curl \
    unzip \
    tar \
    ninja \
    perl \
    make \
    zip \
    pkgconfig \
    linux-headers \
    libsasl \
  && git clone https://github.com/microsoft/vcpkg.git \
  && cd vcpkg \
  && git reset --hard 790910f79f653978f90aadd958abf3c407215552 \
  && chmod 777 bootstrap-vcpkg.sh \
  && ./bootstrap-vcpkg.sh --useSystemBinaries -disableMetrics \
  && ./vcpkg install --triplet x64-linux \
    boost-bimap \
    slikenet \
    sqlite-orm \
    sqlpp11 \
    sqlpp11-connector-sqlite3 \
    spdlog \
    catch2 \
    sparsepp \
    nlohmann-json \
    mongo-cxx-driver \
    simdjson
# docker build . -t skymp/skymp-base:v4
# docker push skymp/skymp-base:v4
