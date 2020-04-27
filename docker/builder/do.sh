#!/usr/bin/env bash

set -e -o pipefail
# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/../../scripts/_config.sh"

DOCKER_IMAGE_NAME=naga-builder-image
DOCKER_CACHE_VOLUME_NAME="naga-builder-cache"

create_cache_volume() {
  docker volume create --name "$DOCKER_CACHE_VOLUME_NAME" >/dev/null
}

remove_cache_volume() {
  docker volume rm "$DOCKER_CACHE_VOLUME_NAME"
}

ORIGINAL_ARGS=("$@")

# poor man's bash flags parsing
# https://stackoverflow.com/a/14203146/84283
POSITIONAL_OPTS=()
while [[ $# -gt 0 ]]; do
  key="$1"

  case $key in
  -v | --verbose)
    VERBOSE=1
    shift
    ;;
  *) # unknown option
    POSITIONAL_OPTS+=("$1")
    shift
    ;;
  esac
done
set -- "${POSITIONAL_OPTS[@]}" # restore positional parameters

cd "$ROOT_DIR/docker/builder"

BUILDER_DIR="$(pwd)"

if [[ "$1" == "build" ]]; then
  echo_cmd docker build -t "$DOCKER_IMAGE_NAME:latest" .
elif [[ "$1" == "clear-caches" ]]; then
  remove_cache_volume
elif [[ "$1" == "clear" ]]; then
  remove_cache_volume
  docker rmi "$DOCKER_IMAGE_NAME"
elif [[ "$1" == "attach" ]]; then
  echo "TODO"
elif [[ "$1" == "enter" ]]; then
  create_cache_volume
  shift
  PREFERRED_SHELL=${1:-fish}
  ./do.sh run enter "$PREFERRED_SHELL"
elif [[ "$1" == "run" ]]; then
  create_cache_volume
  shift
  echo_cmd docker run \
    --name "naga-builder" \
    -v "$DOCKER_CACHE_VOLUME_NAME:/naga-work" \
    -v "$BUILDER_DIR/fs/naga:/naga" \
    --rm \
    -it "$DOCKER_IMAGE_NAME" \
    "$@"
else
  # else fallback to direct command execution
  set -- "${ORIGINAL_ARGS[@]}"
  echo_cmd "$@"
fi
