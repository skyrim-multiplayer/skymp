FROM skymp-dependencies:latest

# Build the project
COPY ./CMakeLists.txt ./.clang-format ./
COPY ./cmake ./cmake
COPY ./chakra-wrapper ./chakra-wrapper
COPY ./skyrim-platform ./skyrim-platform
COPY ./skymp5-client ./skymp5-client
COPY ./skymp5-front ./skymp5-front
COPY ./skymp5-functions-lib ./skymp5-functions-lib
COPY ./skymp5-scripts ./skymp5-scripts
COPY ./client-deps ./client-deps
COPY ./skymp5-server ./skymp5-server
RUN mkdir build \
  && cd build \ 
  && cmake .. \
  && cmake --build . --config Release
