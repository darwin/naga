#!/usr/bin/env bash

set -e -o pipefail
# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/_config.sh"

KNOWN_PYTHON_DEPS=(
  six
  setuptools
)

cd "$ROOT_DIR"

./scripts/create-venv.sh

source "$VENV_DIR/bin/activate"
pip3 install -U pip
pip3 install --upgrade "${KNOWN_PYTHON_DEPS[@]}"
