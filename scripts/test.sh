#!/usr/bin/env bash

set -e -o pipefail
# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/_config.sh"

cd "$ROOT_DIR"

./scripts/prepare-venv.sh

cd tests
activate_python3
echo_cmd python3 -m unittest discover --failfast --locals "$@"
