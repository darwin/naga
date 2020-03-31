#!/usr/bin/env bash

set -e -o pipefail
# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/_config.sh"

ENV1_DEPS=(
  vex
  setuptools
)

ENV3_DEPS=(
  six
  setuptools
)

cd "$ROOT_DIR"

./scripts/create-venv.sh

# shellcheck disable=SC1090
source "$VENV1_DIR/bin/activate"

pip3 install -U pip
pip3 install --upgrade "${ENV1_DEPS[@]}"

vex --path "$VENV3_DIR" pip3 install -U pip
vex --path "$VENV3_DIR" pip3 install --upgrade "${ENV3_DEPS[@]}"
