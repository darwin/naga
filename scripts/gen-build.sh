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

# activate Python3, we should capture build settings from Python3.7
# shellcheck disable=SC1090
source "$VENV_DIR/bin/activate"
export_python_build_settings

# export variables with our prefix
for name in "${!STPYV8_@}"; do
  export "${name?}"
done

# force python2 when working with depot
# some tools like gn.py try to undo virtualenv and get back to system paths
export PATH=$DEPOT_HOME:$VENV2_DIR/bin:/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin

echo_cmd gn gen --verbose "$BUILD_DIR" \
  --args="import(\"//args/${BUILD_CONFIG_NAME}.gn\") $STPYV8_GN_EXTRA_ARGS" $STPYV8_GN_GEN_EXTRA_ARGS

if [[ -z "$STPYV8_GN_GEN_EXTRA_ARGS" ]]; then
  echo_info "in depot shell, you may now use following commands:"
  echo_info "> ninja -C $BUILD_DIR stpyv8"
fi
