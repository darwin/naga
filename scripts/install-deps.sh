#!/usr/bin/env bash

set -e -o pipefail
# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/_config.sh"

cd "$ROOT_DIR"

echo_cmd ./scripts/install-depot.sh

echo_cmd brew install python3 boost-python3

ENV_DEPS=(
  six
  setuptools
)

echo_cmd ./scripts/create-venv.sh
echo_cmd ./scripts/prepare-gn.sh

# shellcheck disable=SC1090
source "$VENV_DIR/bin/activate"

echo_cmd pip install -U pip
echo_cmd pip install --upgrade "${ENV_DEPS[@]}"
