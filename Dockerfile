FROM node:14.13.1-alpine3.12
ENV VCPKG_FORCE_SYSTEM_BINARIES=1

# Install system dependencies
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
  boost \
  boost-dev

# Download data files
RUN mkdir /skyrim_data_dir \
  && cd /skyrim_data_dir \
  && curl https://skyrim-data-files.s3.eu-west-3.amazonaws.com/Dawnguard.esm > Dawnguard.esm \
  && curl https://skyrim-data-files.s3.eu-west-3.amazonaws.com/Dragonborn.esm > Dragonborn.esm \
  && curl https://skyrim-data-files.s3.eu-west-3.amazonaws.com/HearthFires.esm > HearthFires.esm \
  && curl https://skyrim-data-files.s3.eu-west-3.amazonaws.com/Skyrim.esm > Skyrim.esm \
  && curl https://skyrim-data-files.s3.eu-west-3.amazonaws.com/Update.esm > Update.esm \
  && curl https://skyrim-data-files.s3.eu-west-3.amazonaws.com/scripts.zip > scripts.zip \
  && unzip -qq scripts.zip \
  && rm scripts.zip

# This file contains current vcpkg master's commit
# We expect 
COPY .git/modules/vcpkg/refs/heads/master .

RUN git clone https://github.com/skyrim-multiplayer/vcpkg.git \ 
  && cd vcpkg \
  && git checkout $(cat master) \
  && chmod 777 ./bootstrap-vcpkg.sh \
  && ./bootstrap-vcpkg.sh -useSystemBinaries -disableMetrics

# Install vcpkg dependencies
COPY ./vcpkg.json ./vcpkg.json
COPY x64-linux-musl.cmake vcpkg/triplets/
RUN vcpkg/vcpkg --feature-flags=binarycaching,manifests install --triplet x64-linux \
  && rm -r vcpkg/buildtrees/*

# Install node headers
RUN wget -O node-headers.tar.gz https://nodejs.org/dist/v14.13.1/node-v14.13.1-headers.tar.gz \
  && tar -xzf node-headers.tar.gz \
  && rm -r node-headers.tar.gz

# Install npm dependencies
COPY ./package.json ./package.json
COPY ./skymp5-client/package.json ./skymp5-client/package.json
COPY ./skymp5-gamemode/package.json ./skymp5-gamemode/package.json
RUN npm i

# Build scamp_native (except node addon)
COPY ./CMakeLists.txt ./CMakeLists.txt
COPY ./cmake ./cmake
COPY ./scamp_native ./scamp_native
COPY ./.clang-format ./.clang-format

RUN mkdir build \
  && cd build \ 
  && cmake .. -DSKYMP_DANGER_ENABLE_PARTNER_FEATURES=ON -DCMAKE_TOOLCHAIN_PATH=/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-linux
RUN npm run build-cpp-prod

# Build TypeScript part
COPY ./src/back ./src/back
COPY ./test ./test
COPY ./ui ./ui
COPY ./tsconfig.json ./tsconfig.json
COPY ./tsconfig.back.json ./tsconfig.back.json
COPY ./jest.config.js ./jest.config.js
COPY ./skymp5-client ./skymp5-client
COPY ./skymp5-gamemode ./skymp5-gamemode
RUN npm run build-ts

CMD [ "npm", "run", "start" ]