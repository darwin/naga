#!/usr/bin/env bash

set -e -o pipefail
# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/_config.sh"

cd "$ROOT_DIR"

if [[ ! -d "$VENV_DIR" ]]; then
  # create a new clean venv
  python3 -m venv "$VENV_DIR"

  activate_python3
  pip install -U pip
  pip install --upgrade virtualenv
fi

if [[ ! -d "$VENV2_DIR" ]]; then
  # create a new clean venv with python2
  # we need virtualenv which supports creating python2 envs
  activate_python3
  virtualenv -p python2 "$VENV2_DIR"
fi
