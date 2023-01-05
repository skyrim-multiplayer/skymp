#!/usr/bin/env bash

if [ "" == "$1" ]; then
  echo Expected version tag >&2
  exit 1
fi
VERSION_TAG="$1"
shift

set -e
set -x

VCPKG_URL="`git -C vcpkg remote get-url origin`"
VCPKG_COMMIT="`git -C vcpkg rev-parse HEAD`"

sed -i -r -e 's/(skymp-(vcpkg-deps|runtime-base):)[-0-9a-zA-Z]+/\1'"$VERSION_TAG"'/g' ci/github_env_linux ci/deploy/remote/branchctl.sh

podman build . --target=skymp-vcpkg-deps --tag=skymp/skymp-vcpkg-deps:"$VERSION_TAG" --cpu-quota=$((100000*10)) --build-arg VCPKG_URL="$VCPKG_URL" --build-arg VCPKG_COMMIT="$VCPKG_COMMIT"
podman build . --target=skymp-runtime-base --tag=skymp/skymp-runtime-base:"$VERSION_TAG" --cpu-quota=$((100000*10)) --build-arg VCPKG_URL="$VCPKG_URL" --build-arg VCPKG_COMMIT="$VCPKG_COMMIT"

echo -n 'ENTER to push'
read

podman push localhost/skymp/skymp-vcpkg-deps:"$VERSION_TAG" docker.io/skymp/skymp-vcpkg-deps:"$VERSION_TAG"
podman push localhost/skymp/skymp-runtime-base:"$VERSION_TAG" docker.io/skymp/skymp-runtime-base:"$VERSION_TAG"
