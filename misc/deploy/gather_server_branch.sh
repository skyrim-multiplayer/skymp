#!/usr/bin/env bash

set -e
set -x

export DEPLOY_OWNER=skyrim-multiplayer
export DEPLOY_REPO=skymp
export DEPLOY_REMOTE=""
export DEPLOY_GITHUB_TOKEN=""
data="`cd misc/deploy/gather-server-branch && node_modules/.bin/ts-node list_included_pulls.ts`"
msg="Gathering $DEPLOY_BRANCH..."$'\n'
msg+="`echo "$data" | tail -n +2`"  # start from second line
./misc/deploy/call_webhook.sh "$msg"
echo "$data" | head -n 1 | xargs ./misc/deploy/merge_pulls.sh # Take first line and pass it to script. It treats this as a list of PR numbers

export DEPLOY_OWNER=Pospelove
export DEPLOY_REPO=skymp5-patches
export DEPLOY_REMOTE=patches
export DEPLOY_GITHUB_TOKEN=$GITHUB_TOKEN_PATCHES
data="`cd misc/deploy/gather-server-branch && node_modules/.bin/ts-node list_included_pulls.ts`"
msg="Gathering $DEPLOY_BRANCH..."$'\n'
msg+="`echo "$data" | tail -n +2`"  # start from second line
./misc/deploy/call_webhook.sh "$msg"
echo "$data" | head -n 1 | xargs ./misc/deploy/merge_pulls.sh # Take first line and pass it to script. It treats this as a list of PR numbers


git submodule update --init --recursive
