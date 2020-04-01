#!/usr/bin/env bash

set -e -o pipefail
# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/_config.sh"

cd "$ROOT_DIR"

DETECTED_CF_VERSION=$(clang-format --version | cut -d " " -f 3)
if [[ $(ver "$DETECTED_CF_VERSION") < $(ver "9.0.0") ]]; then
  echo_err "clang-format v9.0.0 and later is required"
  echo "your version is '$DETECTED_CF_VERSION' (located at $(command -v clang-format))"
  exit 1
fi

find src -type f -print | grep -E ".*\.(cpp|h)" | xargs clang-format -i --verbose
