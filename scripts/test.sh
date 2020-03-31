#!/usr/bin/env bash

set -e -o pipefail
# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/_config.sh"

TEST_GLOB=${1:-*}

cd "$ROOT_DIR"

./scripts/create-venv.sh

set -x
cd tests

export PATH=$VENV_DIR/bin:/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin
python -m unittest discover -p "$TEST_GLOB"
