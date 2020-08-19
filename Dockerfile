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

RUN apt-get install unzip \
  && mkdir /skyrim_data_dir \ 
  && curl "https://downloader.disk.yandex.ru/disk/b0bd6d250a38af5f493f53c3dd720dbad9e17340e404c9d4ae4d728a32f42019/5f3d836c/VQ6HMMpmBmS5h50I-EPS3siVF5_BZAkJJnl-bZjkAmkynRz-5mDdoFnkSC7A3oQHfua3BPArX1mb67SNmJnRPg%3D%3D?uid=0&filename=Data.zip&disposition=attachment&hash=zFVmg9FGlZJUklfISQgqDOZCre1KAnGuKxhnMAotHqLsXETuzbZtOhIkPGLa6nPFq/J6bpmRyOJonT3VoXnDag%3D%3D&limit=0&content_type=application%2Fzip&owner_uid=1026578774&fsize=264080548&hid=cdfc4976ebc1a8db691ce794476c99af&media_type=compressed&tknv=v2" -o /skyrim_data_dir/Data.zip \
  && cd /skyrim_data_dir \
  && unzip Data.zip

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
