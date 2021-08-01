FROM skymp/skymp-base:v1

RUN pwd
RUn false

# Build the project and install missing vcpkg dependencies if any
COPY . .
RUN rm -rf ./skymp5-server/cmake-js-fetch-build || true \
  && npm cache verify \
  && mkdir -p build \
  && cd build \ 
  && cmake .. -DCMAKE_BUILD_TYPE=Release \
  && cmake --build . --config Release
