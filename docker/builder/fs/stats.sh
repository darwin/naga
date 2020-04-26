#!/usr/bin/env bash

set -e -o pipefail

echo_cmd() {
  echo "> $*"
  "$@"
}

echo "=============="
echo "Environment:"

echo_cmd lsb_release -a
echo_cmd python --version
echo_cmd python3 --version

echo "=============="
