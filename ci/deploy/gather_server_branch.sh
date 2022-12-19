#!/usr/bin/env bash

set -e
set -x

data="`cd ci/deploy/gather-server-branch && node_modules/.bin/ts-node list_included_pulls.ts`"

msg="Gathering $DEPLOY_BRANCH..."$'\n'
msg+="`echo "$data" | tail -n +2`"  # start from second line
./ci/deploy/call_webhook.sh "$msg"

# Take first line and pass it to script. It treats this as a list of PR numbers
echo "$data" | head -n 1 | xargs ./ci/deploy/merge_pulls.sh

git submodule init --update --recursive
