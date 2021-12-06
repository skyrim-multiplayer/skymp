#!/usr/bin/env bash

MAIN_REPO="upstream"
if git remote -vv | grep -q fork; then
  MAIN_REPO="origin"
fi

set -e

merge_pull() {
  git pull --squash "$MAIN_REPO" "pull/$1/head"
  git commit -m "merge pull #$1"
}

while [[ "$1" != "" ]]; do
  merge_pull "$1"
  shift
done
