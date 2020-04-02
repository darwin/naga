#!/usr/bin/env bash

set -e -o pipefail
# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/_config.sh"

echo_cmd cd "$GN_DIR"

symlink_v8_dir() {
  if [[ -L "$1" ]]; then
    rm "$1"
  fi
  if [[ ! -e "$1" ]]; then
    echo_cmd ln -s "v8/$1" "$1"
  fi
}

# relink V8 directories under gn
# don't touch them if files are not symlinks but exist

if [[ -L "v8" ]]; then
  rm "v8"
fi
if [[ ! -e "v8" ]]; then
  echo_cmd ln -s "$V8_HOME" "v8"
fi

symlink_v8_dir "tools"
symlink_v8_dir "third_party"
symlink_v8_dir "testing"
symlink_v8_dir "build"
symlink_v8_dir "build_overrides"