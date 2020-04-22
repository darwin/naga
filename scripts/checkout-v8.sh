#!/usr/bin/env bash

set -e -o pipefail
# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/_config.sh"

cd "$ROOT_DIR"

# see https://v8.dev/docs/source-code

if [[ ! -d "$V8_HOME" ]]; then
  echo_cmd mkdir -p "$V8_HOME"
  cd "$V8_HOME"
  cd ..
  echo_cmd fetch v8
fi

echo_cmd cd "$V8_HOME"
echo_cmd git fetch --tags
echo_cmd git checkout "$NAGA_V8_GIT_TAG"
echo_cmd gclient sync -D

if [[ "$OSTYPE" == "linux"* ]]; then
  echo_info "note: you might want to install V8 build dependencies"
  echo_info "      see https://v8.dev/docs/build step 4"
  echo_info "      ./v8/build/install-build-deps.sh"
fi
