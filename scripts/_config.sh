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
V8_HOME=${V8_HOME:-"$ROOT_DIR/gn/v8"}
DEPOT_HOME=${DEPOT_HOME:-"$ROOT_DIR/depot_tools"}

NAGA_V8_GIT_TAG=${NAGA_V8_GIT_TAG:-8.4.100}

NAGA_V8_GIT_URL=${NAGA_V8_GIT_URL:-"https://chromium.googlesource.com/v8/v8.git"}
NAGA_DEPOT_GIT_URL=${NAGA_DEPOT_GIT_URL:-"https://chromium.googlesource.com/chromium/tools/depot_tools.git"}
NAGA_BASH_COLORS=${NAGA_BASH_COLORS:-auto}
NAGA_GN_EXTRA_ARGS=${NAGA_GN_EXTRA_ARGS}
NAGA_GN_GEN_EXTRA_ARGS=${NAGA_GN_GEN_EXTRA_ARGS}
# by default we reuse buildtools from V8 checkout
# https://groups.google.com/a/chromium.org/forum/#!msg/chromium-dev/TXmL02apluA/5AkiLhNGCAAJ
NAGA_BUILDTOOLS_PATH=${NAGA_BUILDTOOLS_PATH:-$V8_HOME/buildtools}

NAGA_CLANG_FORMAT_PATH=${NAGA_CLANG_FORMAT_PATH:-clang-format}
NAGA_CLANG_TIDY_PATH=${NAGA_CLANG_TIDY_PATH:-clang-tidy}
NAGA_CLANG_TIDY_EXTRA_ARGS=${NAGA_CLANG_TIDY_EXTRA_ARGS}

NAGA_CPYTHON_REPO_DIR=${NAGA_CPYTHON_REPO_DIR:-"$ROOT_DIR/vendor/cpython"}

NAGA_ACTIVE_LOG_LEVEL=${NAGA_ACTIVE_LOG_LEVEL}

# these get initialized on-demand via detect_python_build_settings
NAGA_PYTHON_INCLUDES=${NAGA_PYTHON_INCLUDES}
NAGA_PYTHON_LIBS=${NAGA_PYTHON_LIBS}
NAGA_PYTHON_CFLAGS=${NAGA_PYTHON_CFLAGS}
NAGA_PYTHON_LDFLAGS=${NAGA_PYTHON_LDFLAGS}
NAGA_PYTHON_ABIFLAGS=${NAGA_PYTHON_ABIFLAGS}

# git cache path could speedup depot checkouts/syncs
# as suggested in https://github.com/electron/electron/blob/master/docs/development/build-instructions-gn.md#git_cache_path
NAGA_GIT_CACHE_PATH=${NAGA_GIT_CACHE_PATH:-$ROOT_DIR/.git_cache}
if [[ -n "$NAGA_GIT_CACHE_PATH" ]]; then
  export GIT_CACHE_PATH="${NAGA_GIT_CACHE_PATH}"
  if [[ ! -d "$GIT_CACHE_PATH" ]]; then
    mkdir -p "$GIT_CACHE_PATH"
  fi
fi

# this is for depot tools to be on path during our script execution
export PATH=$DEPOT_HOME:$PATH

# determine if we want to print colors
if [[ $NAGA_BASH_COLORS == auto ]]; then
  if [[ -t 1 ]]; then NAGA_BASH_COLORS=yes; else NAGA_BASH_COLORS=no; fi
fi

export CHROMIUM_BUILDTOOLS_PATH=${NAGA_BUILDTOOLS_PATH}

popd >/dev/null
