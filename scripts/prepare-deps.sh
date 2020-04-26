#!/usr/bin/env bash

set -e -o pipefail
# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/_config.sh"

cd "$ROOT_DIR"

echo_cmd ./scripts/install-depot.sh

ENV_DEPS=(
  six
  setuptools
  python-config
)

echo_cmd ./scripts/prepare-venv.sh

activate_python3

echo_cmd pip install --upgrade "${ENV_DEPS[@]}"
