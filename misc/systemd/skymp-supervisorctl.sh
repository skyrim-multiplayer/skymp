#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -ne 1 ]; then
  echo "Usage: $0 <stop|restart>" >&2
  exit 2
fi

ACTION="$1"
SERVICE_NAME="skymp.service"

case "$ACTION" in
  stop|restart)
    exec /usr/bin/sudo /usr/bin/systemctl "$ACTION" "$SERVICE_NAME"
    ;;
  *)
    echo "Unsupported action: $ACTION" >&2
    exit 2
    ;;
esac
