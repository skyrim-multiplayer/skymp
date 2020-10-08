FROM node:14.13.1-alpine3.10
RUN apk add --no-cache gcc musl-dev g++ cmake gdb git curl unzip tar ninja

# vcpkg
RUN mkdir -p /root/.local/share/pmm/1.4.2/vcpkg-tmp && cd /root/.local/share/pmm/1.4.2/vcpkg-tmp
RUN git clone https://github.com/microsoft/vcpkg.git \
  && cd vcpkg \
  && git reset --hard bff594f7ff8e023592f366b67fd7f57f4fe035e7 \
  && chmod 777 bootstrap-vcpkg.sh \
  && ./bootstrap-vcpkg.sh --useSystemBinaries
RUN cd vcpkg && ./vcpkg install boost-bimap
RUN cd vcpkg && ./vcpkg install slikenet:x64-linux
RUN cd vcpkg && ./vcpkg install sqlite-orm:x64-linux
RUN cd vcpkg && ./vcpkg install spdlog:x64-linux
RUN cd vcpkg && ./vcpkg install catch2:x64-linux
RUN cd vcpkg && ./vcpkg install sparsepp:x64-linux
RUN cd vcpkg && ./vcpkg install nlohmann-json:x64-linux
RUN mv vcpkg /root/.local/share/pmm/1.4.2/vcpkg-bff594f7ff8e023592f366b67fd7f57f4fe035e7

WORKDIR /usr/src/app

COPY package*.json ./
RUN npm i

#COPY ./CMakeLists.txt ./CMakeLists.txt
#COPY ./cmake ./cmake
#COPY ./scamp_native ./scamp_native
#RUN npm run configure
#RUN npm run build-cpp

#COPY . .

#RUN npm run build-ts

#CMD [ "npm", "run", "start" ]
