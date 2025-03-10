FROM skymp/skymp-runtime-base:2c7d8a5

ARG BUILD_TYPE
ARG GAMEMODE_GITHUB_TOKEN
ARG DEPLOY_BRANCH
ENV CI=true

COPY --chown=skymp:skymp . /src

USER skymp
WORKDIR /src

RUN ./build.sh --configure \
    -DBUILD_UNIT_TESTS=OFF \
    -DBUILD_GAMEMODE=ON \
    -DOFFLINE_MODE=OFF \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DGITHUB_TOKEN=$GAMEMODE_GITHUB_TOKEN \
 && ./build.sh --build
