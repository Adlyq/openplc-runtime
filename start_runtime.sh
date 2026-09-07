#!/usr/bin/env bash
#
# Start the OpenPLC runtime (patched local image) directly, bypassing the
# bootloader supervisor so it can never be reverted to the stock image.
#
# Requires: the image built from this repo:
#   openplc-runtime-local:v4.2.2
#
# Usage:  sudo ./start_runtime.sh

set -euo pipefail

IMG="openplc-runtime-local:v4.2.2"
NAME="openplc-runtime"
DATA_DIR="/var/lib/openplc-runtime"

cd "$(dirname "$0")"

if [ "$(id -u)" -ne 0 ]; then
	echo "Please run as root (sudo ./start_runtime.sh)" >&2
	exit 1
fi

echo "[1/4] Checking image ${IMG}..."
if ! docker image inspect "${IMG}" >/dev/null 2>&1; then
	echo "Image ${IMG} not found. Build it first:" >&2
	echo "  docker build --network=host --build-arg RUNTIME_VERSION=v4.2.2 -t ${IMG} ." >&2
	exit 1
fi

echo "[2/4] Stopping the bootloader supervisor (avoids it reverting the runtime)..."
if docker ps -a --format '{{.Names}}' | grep -qx openplc-bootloader; then
	docker update --restart=no openplc-bootloader >/dev/null 2>&1 || true
	docker stop openplc-bootloader >/dev/null 2>&1 || true
fi

echo "[3/4] Removing stale runtime containers..."
for c in "$NAME" interesting_kilby; do
	if docker ps -a --format '{{.Names}}' | grep -qx "$c"; then
		docker rm -f "$c" >/dev/null 2>&1 || true
	fi
done

echo "[4/4] Starting ${NAME} from ${IMG}..."
docker run -d \
	--name "$NAME" \
	--restart unless-stopped \
	--network host \
	--privileged \
	-v /dev:/dev \
	-v "${DATA_DIR}:${DATA_DIR}" \
	-e OPENPLC_PERSISTENT_DATA_DIR="${DATA_DIR}" \
	-e RUNTIME_VERSION=v4.2.2 \
	"${IMG}" >/dev/null

echo "Started:"
docker ps --filter "name=${NAME}" --format '{{.Names}}\t{{.Status}}\t{{.Image}}'
echo
echo "Wait for health, then reach the editor at https://localhost:8443"
