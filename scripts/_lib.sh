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
  if [[ $STPYV8_BASH_COLORS == yes ]]; then
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
  >&2 echo_with_color "$COLOR_RED" "$*"
}

detect_python_build_settings() {
  STPYV8_PYTHON_INCLUDES=${STPYV8_PYTHON_INCLUDES:-$(python3-config --includes)}
  STPYV8_PYTHON_LIBS=${STPYV8_PYTHON_LIBS:-$(python3-config --libs)}
  STPYV8_PYTHON_CFLAGS=${STPYV8_PYTHON_CFLAGS:-$(python3-config --cflags)}
  STPYV8_PYTHON_LDFLAGS=${STPYV8_PYTHON_LDFLAGS:-$(python3-config --ldflags)}
  STPYV8_PYTHON_ABIFLAGS=${STPYV8_PYTHON_ABIFLAGS:-$(python3-config --abiflags)}
}

# https://stackoverflow.com/a/53400482/84283
function ver()
# Description: use for comparisons of version strings.
# $1  : a version string of form 1.2.3.4
# use: (( $(ver 1.2.3.4) >= $(ver 1.2.3.3) )) && echo "yes" || echo "no"
{
    printf "%02d%02d%02d%02d" ${1//./ }
}