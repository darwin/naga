#!/usr/bin/env bash

set -e -o pipefail
# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/_config.sh"

cd "$ROOT_DIR"

if [[ ! -d "$NAGA_GN_WORK_DIR" ]]; then
  echo_err "NAGA_GN_WORK_DIR does not exist, did you run ./scripts/gen-build.sh?"
  echo_err "NAGA_GN_WORK_DIR='$NAGA_GN_WORK_DIR'"
  exit 1
fi

cd "$NAGA_GN_WORK_DIR"

# we should capture build settings from Python3.7
activate_python3
detect_python_build_settings

# export env variables with NAGA_ prefix
export_naga_env

activate_python2

if [[ $# == 0 ]]; then
  export BASH_SILENCE_DEPRECATION_WARNING=1
  # TODO: it would be nice to support user's default shell here
  #       unfortunately I don't know how to prevent/pass init file in generic way
  exec bash --rcfile <(echo "PS1='gn: ';env | sort | grep ^NAGA_;echo \"in '\$(pwd)'\"") -i
else
  echo_info "in depot shell"
  echo_cmd "$@"
fi
