#!/usr/bin/env bash

set -e -o pipefail

export NAGA_WORK_DIR="/naga-work"
export NAGA_LDFLAGS="-L/usr/lib/x86_64-linux-gnu"
export NAGA_INCLUDES="-I/usr/include/x86_64-linux-gnu"

cd "/naga"

if [[ "$1" == "build" ]]; then
  set -x
  ./scripts/prepare-deps.sh
  ./scripts/prepare-v8.sh
  ./scripts/build.sh
  # TODO: remove this, just temporary
  ./scripts/test.sh
elif [[ "$1" == "enter" ]]; then
  shift
  "$@"
else
  echo "don't know what to do"
  echo "got: $*"
fi
