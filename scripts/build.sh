#!/usr/bin/env bash

set -e -o pipefail
# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/_config.sh"

# poor man's bash flags parsing
# https://stackoverflow.com/a/14203146/84283
POSITIONAL_OPTS=()
while [[ $# -gt 0 ]]; do
  key="$1"

  case $key in
  -v | --verbose)
    STPYV8_VERBOSE=1
    shift
    ;;
  *) # unknown option
    POSITIONAL_OPTS+=("$1")
    shift
    ;;
  esac
done
set -- "${POSITIONAL_OPTS[@]}" # restore positional parameters

BUILD_CONFIG=${1:-release}

EXTRA_GEN_BUILD_ARGS=()
if [[ -n "$STPYV8_VERBOSE" ]]; then
  EXTRA_GEN_BUILD_ARGS+=("--verbose")
fi

EXTRA_NINJA_ARGS=()
if [[ -n "$STPYV8_VERBOSE" ]]; then
  EXTRA_NINJA_ARGS+=("-v")
fi

EXTRA_BUILD_ARGS=()
if [[ "$BUILD_CONFIG" == "debug" ]]; then
  EXTRA_BUILD_ARGS+=("--debug")
fi
if [[ -n "$STPYV8_VERBOSE" ]]; then
  EXTRA_BUILD_ARGS+=("--verbose")
fi

EXTRA_INSTALL_ARGS=()
if [[ -n "$STPYV8_VERBOSE" ]]; then
  EXTRA_INSTALL_ARGS+=("--verbose")
fi

OUT_DIR="_out/$STPYV8_V8_GIT_TAG/$BUILD_CONFIG"

cd "$ROOT_DIR"

echo_cmd ./scripts/gen-build.sh "${EXTRA_GEN_BUILD_ARGS[@]}" "$BUILD_CONFIG"

echo_cmd ./scripts/enter_depot_shell.sh ninja "${EXTRA_NINJA_ARGS[@]}" -C "$OUT_DIR" stpyv8

echo_cmd cd "$EXT_DIR"

export STPYV8_V8_GIT_TAG

echo_cmd python3 setup.py build "${EXTRA_BUILD_ARGS[@]}"

echo_cmd python3 setup.py install "${EXTRA_INSTALL_ARGS[@]}" --prefix "$VENV_DIR"
