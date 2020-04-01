#!/usr/bin/env bash

set -e -o pipefail
# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/_config.sh"

cd "$GN_DIR"

# see https://v8.dev/docs/source-code
#     https://v8.dev/docs/build

# see configs/args/* for possible names
BUILD_CONFIG_NAME=${1:-release}
BUILD_DIR=${2:-"_out/$STPYV8_V8_GIT_TAG/$BUILD_CONFIG_NAME"}

# force python2 when working with depot
# some tools like gn.py try to undo virtualenv and get back to system paths
export PATH=$DEPOT_HOME:$VENV2_DIR/bin:/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin

echo_cmd gn gen --verbose "$BUILD_DIR" \
  --args="import(\"//args/${BUILD_CONFIG_NAME}.gn\") $STPYV8_GN_EXTRA_ARGS"

echo_info "in depot shell, you may now use following commands:"
echo_info "> ninja -C $BUILD_DIR stpyv8"
