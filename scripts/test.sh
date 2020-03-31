#!/usr/bin/env bash

set -e -o pipefail
# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/_config.sh"

TEST_GLOB=${1:-*}

cd "$ROOT_DIR"

./scripts/create-venv.sh

source "$VENV_DIR/bin/activate"


set -x
cd tests
python -m unittest discover -p "$TEST_GLOB"