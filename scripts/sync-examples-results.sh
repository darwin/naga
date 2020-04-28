#!/usr/bin/env bash

set -e -o pipefail
# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/_config.sh"

cd "$ROOT_DIR"

if [[ ! -d "$NAGA_EXAMPLES_EXPECTATIONS_DIR" ]]; then
  mkdir -p "$NAGA_EXAMPLES_EXPECTATIONS_DIR"
fi

echo_cmd rm "$NAGA_EXAMPLES_EXPECTATIONS_DIR"/*.txt || :
echo_cmd cp -v "$NAGA_EXAMPLES_RESULTS_DIR"/*.txt "$NAGA_EXAMPLES_EXPECTATIONS_DIR"
