#!/usr/bin/env bash

# if DEPLOY_REMOTE is defined then use it as a remote name
# otherwise use "origin" if it points to skyrim-multiplayer
# otherwise use "upstream"
DEPLOY_REMOTE="${DEPLOY_REMOTE:-}"
if [[ -z "$DEPLOY_REMOTE" ]]; then
  if git remote -vv | grep -e 'origin.*skyrim-multiplayer'; then
    DEPLOY_REMOTE="origin"
  else
    DEPLOY_REMOTE="upstream"
  fi
fi

set -e
set -x

merge_pull() {
  git fetch "$DEPLOY_REMOTE" "pull/$1/head"
  git merge --squash FETCH_HEAD
  git commit -m "merge $DEPLOY_REMOTE pull #$1"
}

while [[ "$1" != "" ]]; do
  merge_pull "$1"
  shift
done
