FROM node:14.13.1-alpine3.12
ENV VCPKG_FORCE_SYSTEM_BINARIES=1

# Install system dependencies and Skyrim data files
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
  && mkdir /skyrim_data_dir \
  && cd /skyrim_data_dir \
  && curl https://skyrim-data-files.s3.eu-west-3.amazonaws.com/Dawnguard.esm > Dawnguard.esm \
  && curl https://skyrim-data-files.s3.eu-west-3.amazonaws.com/Dragonborn.esm > Dragonborn.esm \
  && curl https://skyrim-data-files.s3.eu-west-3.amazonaws.com/HearthFires.esm > HearthFires.esm \
  && curl https://skyrim-data-files.s3.eu-west-3.amazonaws.com/Skyrim.esm > Skyrim.esm \
  && curl https://skyrim-data-files.s3.eu-west-3.amazonaws.com/Update.esm > Update.esm \
  && curl https://skyrim-data-files.s3.eu-west-3.amazonaws.com/scripts.zip > scripts.zip \
  && unzip -qq scripts.zip \
  && rm scripts.zip

# Install vcpkg and ports
# (vcpkg/refs/heads/master contains vcpkg version)
COPY .git/modules/vcpkg/refs/heads/master \
  ./vcpkg.json \
  x64-linux-musl.cmake \
  ./
RUN git clone https://github.com/skyrim-multiplayer/vcpkg.git \ 
  && cd vcpkg \
  && git checkout $(cat master) \
  && chmod 777 ./bootstrap-vcpkg.sh \
  && ./bootstrap-vcpkg.sh -useSystemBinaries -disableMetrics \
  && mv ../x64-linux-musl.cmake ./triplets/x64-linux-musl.cmake \
  && cd .. \
  && vcpkg/vcpkg --feature-flags=binarycaching,manifests install --triplet x64-linux \
  && rm -r vcpkg/buildtrees \
  && rm -r vcpkg/packages \
  && rm -r vcpkg/downloads

# Install npm dependencies and node headers
COPY ./package.json ./package.json
COPY ./skymp5-client/package.json ./skymp5-client/package.json
COPY ./skymp5-gamemode/package.json ./skymp5-gamemode/package.json
COPY ./skymp5-front/package.json ./skymp5-front/package.json
RUN wget -O node-headers.tar.gz https://nodejs.org/dist/v14.13.1/node-v14.13.1-headers.tar.gz \
  && tar -xzf node-headers.tar.gz \
  && rm -r node-headers.tar.gz \
  && npm i \
  && npm i -g parcel@1.12.3 \
  && npm cache clean --force

# Build scamp_native
COPY ./CMakeLists.txt ./.clang-format ./
COPY ./cmake ./cmake
COPY ./scamp_native ./scamp_native
RUN mkdir build \
  && cd build \ 
  && cmake .. -DSKYMP_DANGER_ENABLE_PARTNER_FEATURES=ON -DCMAKE_TOOLCHAIN_PATH=/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-linux \
  && cd .. \
  && npm run build-cpp-prod

# Build TypeScript part
COPY ./src/back ./src/back
COPY ./test ./test
COPY ./tsconfig.json ./tsconfig.back.json ./jest.config.js ./
COPY ./skymp5-client ./skymp5-client
COPY ./skymp5-gamemode ./skymp5-gamemode
COPY ./skymp5-front ./skymp5-front
RUN npm install \
  && npm run build-ts

CMD [ "npm", "run", "start-prod" ]