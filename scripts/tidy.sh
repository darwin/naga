#!/usr/bin/env bash

set -e -o pipefail
# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/_config.sh"

cd "$ROOT_DIR"

./scripts/gen-compile-commands-db.sh

COMMAND="$NAGA_CLANG_TIDY_PATH"

if [[ $# == 0 ]]; then
  # shellcheck disable=SC2086
  find src -type f -print | grep -E ".*\.(cpp|h)" | xargs "$COMMAND" $NAGA_CLANG_TIDY_EXTRA_ARGS "$@"
else
  # shellcheck disable=SC2086
  "$COMMAND" $NAGA_CLANG_TIDY_EXTRA_ARGS "$@"
fi
