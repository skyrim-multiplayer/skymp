FROM node:12

WORKDIR /usr/src/app

RUN wget https://github.com/Kitware/CMake/releases/download/v3.15.2/cmake-3.15.2.tar.gz
RUN tar -zxvf cmake-3.15.2.tar.gz
RUN cd cmake-3.15.2 && ./bootstrap && make && make install
RUN cmake --version

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

RUN apt-get update && apt-get upgrade -y && apt-get install -y build-essential
RUN gcc --version

RUN mkdir /skyrim_data_dir \
  && git clone https://gitlab.com/pospelov/skyrim-dlcs.git skyrim-dlcs \
  && mv skyrim-dlcs/* /skyrim_data_dir

COPY package*.json ./
RUN npm i

COPY ./CMakeLists.txt ./CMakeLists.txt
COPY ./cmake ./cmake
COPY ./scamp_native ./scamp_native
RUN npm run configure
RUN npm run build-cpp

COPY . .

RUN npm run build-ts

CMD [ "npm", "run", "start" ]
