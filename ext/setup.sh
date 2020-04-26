#!/usr/bin/env bash

# this is a setup.py launcher
# we ensure correct Python3 version and prepare naga environment

set -e -o pipefail
# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/../scripts/_config.sh"

activate_python3
export_naga_env

# we would want to use --build-base
# but it is pretty much broken: https://bugs.python.org/issue1011113
# instead let's work from $NAGA_EXT_BUILD_BASE with help of some symlinks
# this also solves littering our source tree with naga.egg-info and other garbage

if [[ ! -d "$NAGA_EXT_BUILD_BASE" ]]; then
  mkdir -p "$NAGA_EXT_BUILD_BASE"
fi

echo_cmd cd "$NAGA_EXT_BUILD_BASE"

if [[ -e "naga" ]]; then
  rm "naga"
fi
ln -s "$EXT_DIR/naga" "naga"

if [[ -e "setup.py" ]]; then
  rm "setup.py"
fi
ln -s "$EXT_DIR/setup.py" "setup.py"

echo_cmd python "setup.py" "$@"
