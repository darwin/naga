#!/usr/bin/env bash

set -e -o pipefail
# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/_config.sh"

cd "$ROOT_DIR"

./scripts/gen-compile-commands-db.sh

COMMAND="$STPYV8_CLANG_TIDY_PATH"

if [[ $# == 0 ]]; then
  find src -type f -print | grep -E ".*\.(cpp|h)" | xargs "$COMMAND" "$@"
else
  "$COMMAND" "$@"
fi