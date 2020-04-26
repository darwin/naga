#!/usr/bin/env bash

set -e -o pipefail
# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/_config.sh"

cd "$ROOT_DIR"

if [[ ! -d "$DEPOT_HOME" ]]; then
  mkdir -p "$DEPOT_HOME"
  echo_cmd git clone "$NAGA_DEPOT_GIT_URL" "$DEPOT_HOME"
fi