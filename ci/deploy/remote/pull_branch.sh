#!/usr/bin/env bash

set -e
set -x

branch="${1:?}"

cd "skymp-server-$branch"
docker stop "skymp-server-$branch" || true
cp ./server-settings.json server/
docker run -d --rm --name="skymp-server-$branch" --network=host \
    -v "$PWD/server:/work" --workdir=/work \
    -u "`id -u`:`id -g`" \
    --cpu-period=50000 --cpu-quota=25000 \
    skymp/skymp-vcpkg-deps ./run.sh

# ^ limited to 50% of CPU: https://stackoverflow.com/a/41552172

timeout 60s docker logs -f "skymp-server-$branch" \
  |& grep -q 'Hello Papyrus'
