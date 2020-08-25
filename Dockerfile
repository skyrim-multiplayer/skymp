FROM gcc:10.2.0

# CMake
RUN wget https://github.com/Kitware/CMake/releases/download/v3.15.2/cmake-3.15.2.tar.gz
RUN tar -zxvf cmake-3.15.2.tar.gz
RUN cd cmake-3.15.2 && ./bootstrap && make && make install
RUN cmake --version

# vcpkg
RUN mkdir -p /root/.local/share/pmm/1.4.2/vcpkg-tmp && cd /root/.local/share/pmm/1.4.2/vcpkg-tmp
RUN git clone https://github.com/microsoft/vcpkg.git \
  && cd vcpkg \
  && git reset --hard bff594f7ff8e023592f366b67fd7f57f4fe035e7 \
  && chmod 777 bootstrap-vcpkg.sh \
  && ./bootstrap-vcpkg.sh
RUN cd vcpkg && ./vcpkg install boost-bimap:x64-linux
RUN cd vcpkg && ./vcpkg install slikenet:x64-linux
RUN cd vcpkg && ./vcpkg install spdlog:x64-linux
RUN cd vcpkg && ./vcpkg install catch2:x64-linux
RUN cd vcpkg && ./vcpkg install sparsepp:x64-linux
RUN cd vcpkg && ./vcpkg install nlohmann-json:x64-linux
RUN mv vcpkg /root/.local/share/pmm/1.4.2/vcpkg-bff594f7ff8e023592f366b67fd7f57f4fe035e7

# NodeJS
RUN apt-get update -yq \
  && apt-get install curl gnupg -yq \
  && curl -sL https://deb.nodesource.com/setup_14.x | bash \
  && apt-get install -yq nodejs

# GDB
RUN apt-get update -yq && \
  apt-get install texinfo -yq
RUN wget "http://ftp.gnu.org/gnu/gdb/gdb-9.2.tar.gz" && tar -xvzf gdb-9.2.tar.gz && \
  mkdir build && cd build && ../gdb-9.2/configure && make && make install && gdb --version

WORKDIR /usr/src/app

COPY package*.json ./
COPY ./src/back/api/package.json ./src/back/api/
RUN npm i
RUN ls ./node_modules
RUN ls ./src/back/api/node_modules
RUN cp ./src/back/api/node_modules ./node_modules

COPY ./CMakeLists.txt ./CMakeLists.txt
COPY ./cmake ./cmake
COPY ./scamp_native ./scamp_native
RUN npm run configure
RUN npm run build-cpp

COPY . .

RUN npm run build-ts

CMD [ "npm", "run", "start" ]
