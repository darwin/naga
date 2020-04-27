#!/usr/bin/env bash

set -e -o pipefail

# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/_config.sh"

NAGA_WORKFLOW_CACHE_DIR=${NAGA_WORKFLOW_CACHE_DIR:-"$NAGA_WORK_DIR/workflow-cache"}
NAGA_CACHE_BACKUP_FILE_PATH="$NAGA_WORKFLOW_CACHE_DIR/$NAGA_DOCKER_BUILDER_CACHE_VOLUME_NAME-backup.tar"
NAGA_CACHE_BACKUP_FILE_DIR="$(dirname "$NAGA_CACHE_BACKUP_FILE_PATH")"

load_docker_volumes() {
  if [[ -f "$NAGA_CACHE_BACKUP_FILE_PATH" ]]; then
    echo "Loading docker volume $NAGA_DOCKER_BUILDER_CACHE_VOLUME_NAME from '$NAGA_CACHE_BACKUP_FILE_PATH'"
    docker volume rm -f "$NAGA_DOCKER_BUILDER_CACHE_VOLUME_NAME"
    docker volume create --name "$NAGA_DOCKER_BUILDER_CACHE_VOLUME_NAME" >/dev/null
    docker run --rm -i \
      -v "$NAGA_DOCKER_BUILDER_CACHE_VOLUME_NAME:/target" \
      busybox tar -xC /target <"$NAGA_CACHE_BACKUP_FILE_PATH"
  else
    echo "Skipping loading docker volume $NAGA_DOCKER_BUILDER_CACHE_VOLUME_NAME"
    echo "because '$NAGA_CACHE_BACKUP_FILE_PATH' does not exist"
  fi
}

save_docker_volumes() {
  echo "Exporting docker volume $NAGA_DOCKER_BUILDER_CACHE_VOLUME_NAME to '$NAGA_CACHE_BACKUP_FILE_PATH'"
  if [[ ! -d "$NAGA_CACHE_BACKUP_FILE_DIR" ]]; then
    mkdir -p "$NAGA_CACHE_BACKUP_FILE_DIR"
  fi
  docker run --rm \
    -v "$NAGA_DOCKER_BUILDER_CACHE_VOLUME_NAME:/source:ro" \
    busybox tar -cC /source . >"$NAGA_CACHE_BACKUP_FILE_PATH"
}

set -x
cd "$ROOT_DIR/docker/builder"

./do.sh build

# our docker image uses a separate volume for its caches (NAGA_DOCKER_BUILDER_CACHE_VOLUME_NAME)
# we try to persist that between github workflow runs by exporting and importing the content as suggested by
# https://stackoverflow.com/a/53622621/84283
# the NAGA_WORKFLOW_CACHE_DIR should be cached by github
load_docker_volumes
./do.sh run build
save_docker_volumes

./do.sh run test
