#!/usr/bin/env bash

set -e -o pipefail
# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/_config.sh"

cd "$ROOT_DIR"

# see https://v8.dev/docs/source-code
V8_HOME_PARENT="$V8_HOME/.."
if [[ ! -d "$V8_HOME" ]]; then
  if [[ ! -d "$V8_HOME_PARENT" ]]; then
    echo_cmd mkdir -p "$V8_HOME_PARENT"
  fi
fi

cd "$V8_HOME_PARENT"
# we have to force fetch to prevent error:
# "Your current directory appears to already contain, or be part of a checkout."
# it complains because we are doing a checkout under our git repo (in vendor/v8)
echo_cmd fetch --force v8 # note that this will also update our repo if it exists and will fetch missing tags

echo_cmd cd "$V8_HOME"
echo_cmd git checkout "$NAGA_V8_GIT_TAG"

activate_python3
silence_gclient_python3_warning
echo_cmd gclient sync -D

if [[ "$OSTYPE" == "linux"* ]]; then
  echo_info "note: you might want to install V8 build dependencies"
  echo_info "      see https://v8.dev/docs/build step 4"
  echo_info "      ./v8/build/install-build-deps.sh"
fi
