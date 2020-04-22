#!/usr/bin/env bash

set -e -o pipefail
# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/_config.sh"

cd "$ROOT_DIR"

TARGETS=${1:-stpyv8_src}

NAGA_GN_GEN_EXTRA_ARGS=--export-compile-commands="$TARGETS"
export NAGA_GN_GEN_EXTRA_ARGS

NAGA_GN_EXTRA_ARGS="$NAGA_GN_EXTRA_ARGS stpyv8_gen_compile_commands=true"
export NAGA_GN_EXTRA_ARGS

echo_cmd ./scripts/gen-build.sh debug _out/ccdb

# the above command should produce "$GN_DIR/_out/ccdb/compile_commands.json"
GEN_CC_JSON="$GN_DIR/_out/ccdb/compile_commands.json"

if [[ ! -f "$GEN_CC_JSON" ]]; then
  echo_err "expected file '$GEN_CC_JSON' not found"
  exit 1
fi

# move the file to the root
REAL_CC_JSON="$ROOT_DIR/compile_commands.json"

sed "s#-Wno-non-c-typedef-for-linkage##" \
  <"$GEN_CC_JSON" >"$REAL_CC_JSON"
