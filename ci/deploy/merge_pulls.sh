#!/usr/bin/env bash

MAIN_REPO="upstream"
if git remote -vv | grep -e 'origin.*skyrim-multiplayer'; then
  MAIN_REPO="origin"
fi

set -e
set -x

merge_pull() {
  git pull --squash --allow-unrelated-histories "$MAIN_REPO" "pull/$1/head"
  git commit -m "merge pull #$1"
}

while [[ "$1" != "" ]]; do
  merge_pull "$1"
  shift
done
