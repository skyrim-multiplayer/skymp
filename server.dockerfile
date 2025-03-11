FROM skymp/skymp-vcpkg-deps:2c7d8a5

ARG BUILD_TYPE
ENV CI=true

WORKDIR /src

COPY . .
COPY ./vcpkg ./vcpkg

RUN ./build.sh --configure \
    -DBUILD_UNIT_TESTS=OFF \
    -DBUILD_GAMEMODE=OFF \
    -DOFFLINE_MODE=OFF \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
 && ./build.sh --build
