#!/usr/bin/env bash

set -e

data="`ci/deploy/gather-indev/node_modules/.bin/ts-node ci/deploy/gather-indev/cli.ts`"

msg=$'Gathering indev...\n'
msg+="`echo "$data" | tail -n +2`"
echo "$msg"
./ci/deploy/call_webhook.sh "$msg"

echo "$data" | head -n 1 | xargs echo ./merge_pulls.sh
