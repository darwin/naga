#!/usr/bin/env bash

set -e -o pipefail
# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/_config.sh"

cd "$ROOT_DIR"

find src -type f -print | grep -E ".*\.(cpp|h)" | xargs clang-format -i --verbose