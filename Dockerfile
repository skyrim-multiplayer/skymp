FROM node:14.13.1-alpine3.12
RUN apk add --no-cache gcc musl-dev g++ cmake gdb git curl unzip tar ninja perl

# vcpkg
RUN mkdir -p /root/.local/share/pmm/1.4.2/vcpkg-tmp && cd /root/.local/share/pmm/1.4.2/vcpkg-tmp
RUN git clone https://github.com/microsoft/vcpkg.git \
  && cd vcpkg \
  && git reset --hard 790910f79f653978f90aadd958abf3c407215552 \
  && chmod 777 bootstrap-vcpkg.sh \
  && ./bootstrap-vcpkg.sh --useSystemBinaries
RUN cd vcpkg && ./vcpkg install --triplet x64-linux boost-bimap
RUN apk add --no-cache make
RUN cd vcpkg && ./vcpkg install --triplet x64-linux slikenet
RUN cd vcpkg && ./vcpkg install --triplet x64-linux sqlite-orm
RUN cd vcpkg && ./vcpkg install --triplet x64-linux sqlpp11
RUN cd vcpkg && ./vcpkg install --triplet x64-linux sqlpp11-connector-sqlite3
RUN cd vcpkg && ./vcpkg install --triplet x64-linux spdlog
RUN cd vcpkg && ./vcpkg install --triplet x64-linux catch2
RUN cd vcpkg && ./vcpkg install --triplet x64-linux sparsepp
RUN cd vcpkg && ./vcpkg install --triplet x64-linux nlohmann-json
RUN mv vcpkg /root/.local/share/pmm/1.4.2/vcpkg-790910f79f653978f90aadd958abf3c407215552

WORKDIR /usr/src/app

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
