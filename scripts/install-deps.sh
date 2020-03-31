#!/usr/bin/env bash

set -e -o pipefail
# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/_config.sh"

brew install python3 boost-python3

ENV_DEPS=(
  six
  setuptools
)

cd "$ROOT_DIR"

./scripts/create-venv.sh

# shellcheck disable=SC1090
source "$VENV_DIR/bin/activate"

set -x
pip install -U pip
pip install --upgrade "${ENV_DEPS[@]}"