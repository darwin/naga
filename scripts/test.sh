#!/usr/bin/env bash

set -e -o pipefail
# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/_config.sh"

cd "$ROOT_DIR"

./scripts/prepare-venv.sh

set -x
cd tests

export PATH=$VENV_DIR/bin:/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin
python3 -m unittest discover --failfast --locals "$@"
