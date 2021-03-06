#!/usr/bin/env bash

# this is an attempt to get portable way how to get absolute path path with symlink resolution
get_absolute_path_of_existing_file() {
  local dir base abs_dir
  dir=$(dirname "$1")
  base=$(basename "$1")
  pushd "$dir" >/dev/null || exit 21
  abs_dir=$(pwd -P)
  popd >/dev/null || exit 21
  echo "$abs_dir/$base"
}

# https://stackoverflow.com/a/28938235/84283
COLOR_OFF='\033[0m'       # Text Reset
COLOR_BLACK='\033[0;30m'  # Black
COLOR_RED='\033[0;31m'    # Red
COLOR_GREEN='\033[0;32m'  # Green
COLOR_YELLOW='\033[0;33m' # Yellow
COLOR_BLUE='\033[0;34m'   # Blue
COLOR_PURPLE='\033[0;35m' # Purple
COLOR_CYAN='\033[0;36m'   # Cyan
COLOR_WHITE='\033[0;37m'  # White

echo_with_color() {
  if [[ $NAGA_BASH_COLORS == yes ]]; then
    echo -e "${1}${2}${COLOR_OFF}"
  else
    echo -e "${2}"
  fi
}

echo_cmd() {
  echo_with_color "$COLOR_BLUE" "> $*"
  "$@"
}

echo_info() {
  echo_with_color "$COLOR_YELLOW" "$*"
}

echo_err() {
  echo_with_color >&2 "$COLOR_RED" "$*"
}

detect_python_build_settings() {
  NAGA_PYTHON_INCLUDES=${NAGA_PYTHON_INCLUDES:-$(python3-config --includes)}
  NAGA_PYTHON_LIBS=${NAGA_PYTHON_LIBS:-$(python3-config --libs)}
  NAGA_PYTHON_CFLAGS=${NAGA_PYTHON_CFLAGS:-$(python3-config --cflags)}
  NAGA_PYTHON_LDFLAGS=${NAGA_PYTHON_LDFLAGS:-$(python3-config --ldflags)}
  NAGA_PYTHON_ABIFLAGS=${NAGA_PYTHON_ABIFLAGS:-$(python3-config --abiflags)}
}

# https://stackoverflow.com/a/53400482/84283
ver() { # Description: use for comparisons of version strings.
  # $1  : a version string of form 1.2.3.4
  # use: (( $(ver 1.2.3.4) >= $(ver 1.2.3.3) )) && echo "yes" || echo "no"
  printf "%02d%02d%02d%02d" ${1//./ }
}

export_naga_env() {
  # export variables with our prefix
  for name in "${!NAGA_@}"; do
    export "${name?}"
  done
}

unset_python_home() {
  if [ -n "${PYTHONHOME:-}" ]; then
    unset PYTHONHOME
    export PYTHONHOME
  fi
}

activate_python() {
  unset_python_home

  VIRTUAL_ENV="$1"
  export VIRTUAL_ENV

  PATH="$VIRTUAL_ENV/bin:$PATH"
  export PATH

  echo_info "Active Python virtualenv: $VIRTUAL_ENV"
}

activate_python3() {
  activate_python "$VENV_DIR"
}

activate_python2() {
  activate_python "$VENV2_DIR"
}

silence_gclient_python3_warning() {
  GCLIENT_PY3=1
  export GCLIENT_PY3
}

