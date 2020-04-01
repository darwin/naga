#!/usr/bin/env bash

set -e -o pipefail
# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/_config.sh"

BUILD_CONFIG=${1:-release}

cd "$ROOT_DIR"

echo_cmd ./scripts/gen-build.sh "$BUILD_CONFIG"

OUT_DIR="_out/$STPYV8_V8_GIT_TAG/$BUILD_CONFIG"

echo_cmd ./scripts/enter_depot_shell.sh ninja -C "$OUT_DIR" stpyv8

echo_cmd cd "$EXT_DIR"

EXTRA_BUILD_ARGS=()
if [[ "$BUILD_CONFIG" == "debug" ]]; then
  EXTRA_BUILD_ARGS+=("--debug")
fi

echo_cmd python setup.py build "${EXTRA_BUILD_ARGS[@]}"

echo_cmd python setup.py install --prefix "$VENV_DIR"
