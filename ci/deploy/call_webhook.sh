#!/usr/bin/env bash

if [[ "$DEPLOY_STATUS_WEBHOOK" = "" ]]; then
  echo no webhook
  exit 0
fi

if [[ "$1" = "" ]]; then
  echo no message
  exit 1
fi

curl "$DEPLOY_STATUS_WEBHOOK" -H 'content-type: application/json' \
    --data "`echo "$1" | jq --raw-input '{content: .}'`" \
  || echo 'WARNING: webhook error'
