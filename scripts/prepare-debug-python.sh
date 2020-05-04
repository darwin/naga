#!/usr/bin/env bash

# this script builds debug build of Python and installs it into VENV_DIR
# https://pythonextensionpatterns.readthedocs.io/en/latest/debugging/debug_python.html

# when running with this version of python, you must build naga extension with Py_DEBUG
# https://github.com/pybind/pybind11/pull/1438

set -e -o pipefail
# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/_config.sh"

cd "$ROOT_DIR"

if [ "$(uname)" != "Darwin" ]; then
  echo_err "this was only tested under macOS"
  exit 1
fi

if [[ ! -d "$NAGA_CPYTHON_REPO_DIR" ]]; then
  echo_err "CPython source directory is expected at '$NAGA_CPYTHON_REPO_DIR' (change it via NAGA_CPYTHON_REPO_DIR)"
  exit 2
fi

echo_cmd cd "$NAGA_CPYTHON_REPO_DIR"

# macOS: use openssl from homebrew
# brew install tcl-tk openssl

# --enable-framework is important for macOS, without it our extension would crash on import

CONFIGURE_FLAGS=(
  "--prefix=$VENV_DIR"
  --with-pydebug
  --with-trace-refs
  --without-gcc
  --disable-ipv6
  --with-assertions
  #  --with-address-sanitizer
  #  --with-memory-sanitizer
  "--enable-framework=$VENV_DIR/Frameworks"
  "--with-openssl=$(brew --prefix openssl)"
)

export LDFLAGS="$LDFLAGS -L/usr/local/opt/tcl-tk/lib"
export CPPFLAGS="$CPPFLAGS -I/usr/local/opt/tcl-tk/include"

echo_cmd ./configure "${CONFIGURE_FLAGS[@]}"
echo_cmd make -j
echo_cmd make install # will use the VENV_DIR prefix above
