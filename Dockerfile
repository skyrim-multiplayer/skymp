FROM skymp/skymp-base:v4

RUN mkdir /skyrim_data_dir \
  && cd /skyrim_data_dir \
  && curl https://skyrim-data-files.s3.eu-west-3.amazonaws.com/Dawnguard.esm > Dawnguard.esm \
  && curl https://skyrim-data-files.s3.eu-west-3.amazonaws.com/Dragonborn.esm > Dragonborn.esm \
  && curl https://skyrim-data-files.s3.eu-west-3.amazonaws.com/HearthFires.esm > HearthFires.esm \
  && curl https://skyrim-data-files.s3.eu-west-3.amazonaws.com/Skyrim.esm > Skyrim.esm \
  && curl https://skyrim-data-files.s3.eu-west-3.amazonaws.com/Update.esm > Update.esm \
  && curl https://skyrim-data-files.s3.eu-west-3.amazonaws.com/scripts.zip > scripts.zip \
  && unzip -qq scripts.zip \
  && rm scripts.zip

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
