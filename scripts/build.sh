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
    NAGA_VERBOSE=1
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
if [[ -n "$NAGA_VERBOSE" ]]; then
  EXTRA_GEN_BUILD_ARGS+=("--verbose")
fi

EXTRA_NINJA_ARGS=()
if [[ -n "$NAGA_VERBOSE" ]]; then
  EXTRA_NINJA_ARGS+=("-v")
fi

EXTRA_BUILD_ARGS=()
if [[ "$BUILD_CONFIG" == "debug" ]]; then
  EXTRA_BUILD_ARGS+=("--debug")
fi
if [[ -n "$NAGA_VERBOSE" ]]; then
  EXTRA_BUILD_ARGS+=("--verbose")
fi

EXTRA_INSTALL_ARGS=()
if [[ -n "$NAGA_VERBOSE" ]]; then
  EXTRA_INSTALL_ARGS+=("--verbose")
fi

OUT_DIR="$NAGA_GN_OUT_DIR/$NAGA_V8_GIT_TAG/$BUILD_CONFIG"

cd "$ROOT_DIR"

echo_cmd ./scripts/gen-build.sh "${EXTRA_GEN_BUILD_ARGS[@]}" "$BUILD_CONFIG"
echo_cmd ./scripts/enter-depot-shell.sh ninja "${EXTRA_NINJA_ARGS[@]}" -C "$OUT_DIR" naga

echo_cmd cd "$EXT_DIR"
echo_cmd ./setup.sh build "${EXTRA_BUILD_ARGS[@]}"
echo_cmd ./setup.sh install "${EXTRA_INSTALL_ARGS[@]}" --prefix "$VENV_DIR"
