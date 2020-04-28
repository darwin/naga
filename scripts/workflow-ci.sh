#!/usr/bin/env bash

set -e -o pipefail

# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/_config.sh"

set -x
cd "$ROOT_DIR/docker/builder"

./do.sh build
./do.sh run build
./do.sh run test
./do.sh run test-examples
