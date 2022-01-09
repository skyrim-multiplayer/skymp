#!/usr/bin/env bash

if [[ "$DEPLOY_STATUS_WEBHOOK" = "" ]]; then
  echo no webhook
  exit 0
fi

if [[ "$1" = "" ]]; then
  echo no message
  exit 1
fi

# Concatenating a common prefix and all args. E.g.:
# `./call_webhook.sh $'This is the first line\n' 'This is ' 'the second line'`
msg="[DEPLOY $DEPLOY_BRANCH] "
while [[ "$1" != "" ]]; do
  msg+="$1"
  shift
done

curl -sS "$DEPLOY_STATUS_WEBHOOK" -H 'content-type: application/json' \
    --data "`echo "$msg" | jq --raw-input --slurp '{content: .}'`" \
  || echo 'WARNING: webhook error'
