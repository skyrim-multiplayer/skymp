FROM skymp/skymp-vcpkg-deps:2c7d8a5

ARG BUILD_TYPE
ARG GAMEMODE_GITHUB_TOKEN
ARG DEPLOY_BRANCH
ENV CI=true

USER skymp
WORKDIR /src

COPY --chown=skymp:skymp . /src

RUN ./build.sh --configure \
    -DBUILD_UNIT_TESTS=OFF \
    -DBUILD_GAMEMODE=ON \
    -DOFFLINE_MODE=OFF \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DGITHUB_TOKEN=$GAMEMODE_GITHUB_TOKEN \
 && ./build.sh --build
