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
  -h | --help)
    NAGA_SHOW_USAGE=1
    shift
    ;;
  -v | --verbose)
    NAGA_VERBOSE=1
    shift
    ;;
  --no-env)
    NAGA_DONT_INHERIT_ENV=1
    shift
    ;;
  --)
    shift
    POSITIONAL_OPTS+=("$@")
    NO_ARGS=()
    set -- "${NO_ARGS[@]}"
    ;;
  *) # unknown option
    POSITIONAL_OPTS+=("$1")
    shift
    ;;
  esac
done
set -- "${POSITIONAL_OPTS[@]}" # restore positional parameters

if [[ -n "$NAGA_SHOW_USAGE" ]]; then
  cat <<EOF
A convenience wrapper for Naga builder Dockerfile.

Usage:

  ./do.sh command [args]

Commands:

  build [args]  - build the docker image, args will be passed to the docker build
  clear         - clear all generated docker images/volumes
  clear-caches  - clear only docker volumes holding caches
  enter [cmd]   - enter built docker image with custom command,
                  e.g. use \`./do.sh enter bash\` to use bash shell interactively
  run [args]    - run built docker image with rest arguments passed
                  into builder.sh running inside the container.

Run sub-commands:

  run build [args]   - build and install naga library inside the container
                       args will be passed to the scripts/build.sh script
                       e.g. \`./do.sh run build debug\` will build it in debug mode (defaults to release)
  run test [args]    - run test script on built naga library inside the container
                       args will be passed to the scripts/test.sh script
                       e.g. \`./do.sh run test -k testJavascriptWrapper\` will run only particular test

Discussion:

  Environment

    It is possible to customize the docker behaviour with env variables.
    Please note that we take all NAGA_ prefixed env variables and "export" them into the container.
    This is handy for example for setting NAGA_V8_GIT_TAG or tweaking other settings.
    Just be aware that not all NAGA env variables will have effect, some variables are hard-coded
    in the container for its proper function, for example NAGA_WORK_DIR is set internally to have
    control over work dir location inside the container.

EOF
  exit 0
fi

cd "$ROOT_DIR/docker/builder"

DOCKER_ENV_FILE="$NAGA_WORK_DIR/docker/builder/env-file"

# generate list of inherited env vars
mkdir -p "$(dirname "$DOCKER_ENV_FILE")" || :
if [[ -z "$NAGA_DONT_INHERIT_ENV" ]]; then
  env | grep "^NAGA_" >"$DOCKER_ENV_FILE" || :
else
  rm "$DOCKER_ENV_FILE"
  touch "$DOCKER_ENV_FILE"
fi

if [[ -n "$NAGA_VERBOSE" ]]; then
  if [[ -z "$NAGA_DONT_INHERIT_ENV" ]]; then
    echo "exported env variables:"
    cat "$DOCKER_ENV_FILE"
  else
    echo "no env variables exported because of --no-env flag"
  fi
fi

BUILDER_DIR="$(pwd)"

if [[ "$1" == "build" ]]; then
  shift
  echo_cmd docker build -t "$DOCKER_IMAGE_NAME:latest" . "$@"
elif [[ "$1" == "clear-caches" ]]; then
  shift
  remove_cache_volume
elif [[ "$1" == "clear" ]]; then
  shift
  remove_cache_volume
  docker rmi "$DOCKER_IMAGE_NAME"
elif [[ "$1" == "attach" ]]; then
  shift
  echo "TODO"
elif [[ "$1" == "enter" ]]; then
  shift
  create_cache_volume
  PREFERRED_SHELL=${1:-fish}
  ./do.sh run enter "$PREFERRED_SHELL"
elif [[ "$1" == "run" ]]; then
  shift
  create_cache_volume
  echo_cmd docker run \
    --name "naga-builder" \
    --env-file "$DOCKER_ENV_FILE" \
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
