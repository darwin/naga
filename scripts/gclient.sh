#!/usr/bin/env bash

set -e -o pipefail
# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/_config.sh"

cd "$V8_HOME"

source "$VENV_DIR/bin/activate"

# force homebrew's python2 when working with depot
export PATH=/usr/local/opt/python@2/bin:$DEPOT_HOME:$PATH

set -x
exec gclient "$@"
