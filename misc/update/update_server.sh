#!/usr/bin/env sh
set -eu

usage() {
  cat <<'EOF'
Usage: ./update_server.sh --package /path/to/release.tar.gz [--install-dir /path/to/server] [--start-after]

Updates an existing SkyMP server installation in place while preserving:
- data/
- server-settings.json
- server-settings-dump.json
- server-settings-merged.json

The script creates a timestamped backup under ./backups before copying new files.
EOF
}

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
PACKAGE_PATH=""
INSTALL_DIR="$SCRIPT_DIR"
START_AFTER=0

while [ "$#" -gt 0 ]; do
  case "$1" in
    --package)
      PACKAGE_PATH=${2:-}
      shift 2
      ;;
    --install-dir)
      INSTALL_DIR=${2:-}
      shift 2
      ;;
    --start-after)
      START_AFTER=1
      shift
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

if [ -z "$PACKAGE_PATH" ]; then
  echo "Missing required --package argument" >&2
  usage >&2
  exit 1
fi

if [ ! -f "$PACKAGE_PATH" ]; then
  echo "Package not found: $PACKAGE_PATH" >&2
  exit 1
fi

INSTALL_DIR="$(CDPATH= cd -- "$INSTALL_DIR" && pwd)"
TIMESTAMP="$(date +%Y%m%d-%H%M%S)"
BACKUP_DIR="$INSTALL_DIR/backups/update-$TIMESTAMP"
TMP_DIR="$(mktemp -d)"

cleanup() {
  rm -rf "$TMP_DIR"
}
trap cleanup EXIT INT TERM

extract_archive() {
  case "$PACKAGE_PATH" in
    *.tar.gz|*.tgz)
      tar -xzf "$PACKAGE_PATH" -C "$TMP_DIR"
      ;;
    *.zip)
      if command -v unzip >/dev/null 2>&1; then
        unzip -q "$PACKAGE_PATH" -d "$TMP_DIR"
      else
        tar -xf "$PACKAGE_PATH" -C "$TMP_DIR"
      fi
      ;;
    *)
      echo "Unsupported package format: $PACKAGE_PATH" >&2
      exit 1
      ;;
  esac
}

resolve_package_root() {
  if [ -d "$TMP_DIR/dist/server/dist_back" ]; then
    echo "$TMP_DIR/dist/server"
    return
  fi

  if [ -d "$TMP_DIR/server/dist_back" ]; then
    echo "$TMP_DIR/server"
    return
  fi

  if [ -d "$TMP_DIR/dist_back" ]; then
    echo "$TMP_DIR"
    return
  fi

  echo "Could not locate extracted server root inside package" >&2
  exit 1
}

stop_server() {
  if pgrep -f 'dist_back/skymp5-server\.js' >/dev/null 2>&1; then
    echo "[SkyMP] Stopping existing server process..."
    pkill -f 'dist_back/skymp5-server\.js' || true
    sleep 1
  fi
}

backup_path_if_exists() {
  src_path="$1"
  if [ -e "$src_path" ]; then
    mkdir -p "$BACKUP_DIR"
    cp -R "$src_path" "$BACKUP_DIR/"
  fi
}

copy_release_files() {
  package_root="$1"

  if command -v rsync >/dev/null 2>&1; then
    rsync -a \
      --exclude 'data' \
      --exclude 'server-settings.json' \
      --exclude 'server-settings-dump.json' \
      --exclude 'server-settings-merged.json' \
      "$package_root/" "$INSTALL_DIR/"
    return
  fi

  for path in "$package_root"/*; do
    name="$(basename "$path")"
    case "$name" in
      data|server-settings.json|server-settings-dump.json|server-settings-merged.json)
        continue
        ;;
    esac
    cp -R "$path" "$INSTALL_DIR/"
  done
}

start_server() {
  echo "[SkyMP] Starting server..."
  nohup "$INSTALL_DIR/launch_server.sh" >/dev/null 2>&1 &
}

echo "[SkyMP] Extracting package: $PACKAGE_PATH"
extract_archive
PACKAGE_ROOT="$(resolve_package_root)"

echo "[SkyMP] Creating backup in $BACKUP_DIR"
backup_path_if_exists "$INSTALL_DIR/data"
backup_path_if_exists "$INSTALL_DIR/server-settings.json"
backup_path_if_exists "$INSTALL_DIR/server-settings-dump.json"
backup_path_if_exists "$INSTALL_DIR/server-settings-merged.json"

stop_server

echo "[SkyMP] Copying new release files into $INSTALL_DIR"
copy_release_files "$PACKAGE_ROOT"

if [ "$START_AFTER" -eq 1 ]; then
  start_server
else
  echo "[SkyMP] Update finished. Start the server with ./launch_server.sh"
fi

echo "[SkyMP] Backup kept at: $BACKUP_DIR"