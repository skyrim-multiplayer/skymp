#!/usr/bin/env bash

set -e
set -x

action="${1:?}"
branch="${2:?}"

cd "skymp-server-$branch"
docker stop "skymp-server-$branch" || true

if [[ "$action" == "stop" ]]; then
  exit 0
fi

cp ./server-settings.json server/
docker run -d --rm --name="skymp-server-$branch" --network=host \
    -v "$PWD/server:/work" --workdir=/work \
    -u "`id -u`:`id -g`" \
    --cpu-period=50000 --cpu-quota=25000 \
    skymp/skymp-vcpkg-deps ./run.sh
# ^ limited to 50% of CPU: https://stackoverflow.com/a/41552172
# TODO(#584): replace vcpkg-deps with lite runtime image

timeout 7m docker logs -f "skymp-server-$branch" \
  |& grep -q 'AttachSaveStorage took'
