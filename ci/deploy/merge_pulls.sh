#!/usr/bin/env bash

MAIN_REPO="upstream"
if git remote -vv | grep -e 'origin.*skyrim-multiplayer'; then
  MAIN_REPO="origin"
fi

set -e
set -x

merge_pull() {
  git fetch "$MAIN_REPO" "pull/$1/head"
  git merge --squash FETCH_HEAD
  git commit -m "merge pull #$1"
}

while [[ "$1" != "" ]]; do
  merge_pull "$1"
  shift
done
