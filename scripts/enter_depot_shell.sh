#!/usr/bin/env bash

set -e -o pipefail
# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/_config.sh"

cd "$ROOT_DIR"

# activate Python3, we should capture build settings from Python3.7
source "$VENV_DIR/bin/activate"
export_python_build_settings

# export variables with our prefix
for name in "${!STPYV8_@}"; do
  export "${name?}"
done

# this will inherit PATH to DEPOT_HOME
cd "$GN_DIR"
# export Python2 path so that depot's gn can properly work
export PATH=$DEPOT_HOME:$VENV2_DIR/bin:/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin

if [[ $# == 0 ]]; then
  exec bash --rcfile <(echo "PS1='gn: ';env | sort | grep ^STPYV8_") -i
else
  echo_info "in depot shell"
  echo_cmd "$@"
fi
