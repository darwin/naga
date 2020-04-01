#!/usr/bin/env bash

set -e -o pipefail

pushd "$(dirname "${BASH_SOURCE[0]}")/.." >/dev/null

source "./scripts/_lib.sh"

ROOT_DIR=$(pwd -P)
# shellcheck disable=SC2034
BUILD_DIR="$ROOT_DIR/.build"
# shellcheck disable=SC2034
GN_DIR="$ROOT_DIR/gn"
# shellcheck disable=SC2034
EXT_DIR="$ROOT_DIR/ext"
VENV_DIR="$ROOT_DIR/.venv"
# shellcheck disable=SC2034
VENV2_DIR="$ROOT_DIR/.venv2"
# shellcheck disable=SC2034
VENV_PACKAGES_DIR="$VENV_DIR/lib/python3.7/site-packages"

# note: this must be kept in sync with setting.py
V8_HOME=${V8_HOME:-v8}
DEPOT_HOME=${DEPOT_HOME:-depot_tools}

STPYV8_V8_GIT_TAG=${STPYV8_V8_GIT_TAG:-8.3.104}

STPYV8_V8_GIT_URL=${STPYV8_V8_GIT_URL:-"https://chromium.googlesource.com/v8/v8.git"}
STPYV8_DEPOT_GIT_URL=${STPYV8_DEPOT_GIT_URL:-"https://chromium.googlesource.com/chromium/tools/depot_tools.git"}
STPYV8_BASH_COLORS=${STPYV8_BASH_COLORS:-auto}
STPYV8_GN_EXTRA_ARGS=${STPYV8_GN_EXTRA_ARGS}
# by default we reuse buildtools from V8 checkout
# https://groups.google.com/a/chromium.org/forum/#!msg/chromium-dev/TXmL02apluA/5AkiLhNGCAAJ
STPYV8_BUILDTOOLS_PATH=${STPYV8_BUILDTOOLS_PATH:-$V8_HOME/buildtools}

# git cache path could speedup depot checkouts/syncs
# as suggested in https://github.com/electron/electron/blob/master/docs/development/build-instructions-gn.md#git_cache_path
STPYV8_GIT_CACHE_PATH=${STPYV8_GIT_CACHE_PATH:-$ROOT_DIR/.git_cache}
if [[ -n "$STPYV8_GIT_CACHE_PATH" ]]; then
  export GIT_CACHE_PATH="${STPYV8_GIT_CACHE_PATH}"
  if [[ ! -d "$GIT_CACHE_PATH" ]]; then
    mkdir -p "$GIT_CACHE_PATH"
  fi
fi

# this is for depot tools to be on path during our script execution
export PATH=$DEPOT_HOME:$PATH

# determine if we want to print colors
if [[ $STPYV8_BASH_COLORS == auto ]]; then
  if [[ -t 1 ]]; then STPYV8_BASH_COLORS=yes; else STPYV8_BASH_COLORS=no; fi
fi

export CHROMIUM_BUILDTOOLS_PATH=${STPYV8_BUILDTOOLS_PATH}

popd >/dev/null
