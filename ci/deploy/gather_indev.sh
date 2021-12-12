#!/usr/bin/env bash

set -e

data="`cd ci/deploy/gather-indev && node_modules/.bin/ts-node list_included_pulls.ts`"

msg=$'Gathering indev...\n'
msg+="`echo "$data" | tail -n +2`"
echo "$msg"
./ci/deploy/call_webhook.sh "$msg"

echo "$data" | head -n 1 | xargs ./ci/deploy/merge_pulls.sh
