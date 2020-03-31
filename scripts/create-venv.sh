#!/usr/bin/env bash

set -e -o pipefail
# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/_config.sh"

cd "$ROOT_DIR"

if [[ ! -d "$VENV1_DIR" ]]; then
  # create a new clean venv
  virtualenv -p python2.7 "$VENV1_DIR"
fi

if [[ ! -d "$VENV2_DIR" ]]; then
  # create a new clean venv
  virtualenv -p python2.7 "$VENV2_DIR"
fi

if [[ ! -d "$VENV3_DIR" ]]; then
  # create a new clean venv
  virtualenv -p python3.7 "$VENV3_DIR"
fi
