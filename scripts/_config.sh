#!/usr/bin/env bash

# this is an attempt to get portable way how to get absolute path path with symlink resolution
get_absolute_path_of_existing_file() {
  local dir base abs_dir
  dir=$(dirname "$1")
  base=$(basename "$1")
  pushd "$dir" >/dev/null || exit 21
  abs_dir=$(pwd -P)
  popd >/dev/null || exit 21
  echo "$abs_dir/$base"
}

pushd "$(dirname "${BASH_SOURCE[0]}")/.." >/dev/null || exit 11

ROOT_DIR=$(pwd -P)
# shellcheck disable=SC2034
BUILD_DIR="$ROOT_DIR/.build"
# the whole python situation is a deep mess
# I have to use vex because of ill-designed virtualenv, see https://datagrok.org/python/activate/
#
# venv1 is base for our tools and vex
# venv2 is with python2 for depot
# venv3 is for building stpyv8 and its deps
VENV1_DIR="$ROOT_DIR/.venv1"
VENV2_DIR="$ROOT_DIR/.venv2"
VENV3_DIR="$ROOT_DIR/.venv3"
VENV3_PACKAGES_DIR="$VENV3_DIR/lib/python3.7/site-packages"

# note: this must be kept in sync with setting.py
V8_HOME=${V8_HOME:-v8}
DEPOT_HOME=${DEPOT_HOME:-depot_tools}

popd >/dev/null || exit 11
