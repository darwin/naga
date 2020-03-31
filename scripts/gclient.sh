#!/usr/bin/env bash

set -e -o pipefail
# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/_config.sh"

cd "$V8_HOME"

# shellcheck disable=SC1090
source "$VENV1_DIR/bin/activate"
set -x

export GCLIENT_PY3=0
exec vex --path "$VENV1_DIR" gclient "$@"
