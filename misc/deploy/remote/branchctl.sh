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

docker rm "skymp-server-$branch" || true

docker run -d --restart=always --name="skymp-server-$branch" --network=host \
    -v "/var/crash:/var/crash" \
    -v "$PWD/server:/work" --workdir=/work \
    -u "`id -u`:`id -g`" \
    --cpu-period=50000 --cpu-quota=25000 \
    --cap-add=SYS_PTRACE \
    skymp/skymp-runtime-base:7a47553 ./run.sh
# ^ limited to 50% of CPU: https://stackoverflow.com/a/41552172

# This looks a bit ugly, but apparently is more fault-tolerant than older version:
# docker logs -f ... |& grep -q ...
for ((t = 0; t < 150; t += 5)); do
    if docker logs "skymp-server-$branch" \
            |& grep -q 'AttachSaveStorage took'; then
        echo health check success
        exit 0
    fi
    sleep 5
done

echo health check failed
exit 1
