#!/usr/bin/env bash

set -e -o pipefail
# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/_config.sh"

cd "$ROOT_DIR"

./scripts/prepare-venv.sh

cd examples
activate_python3

mkdir -p "$NAGA_EXAMPLES_RESULTS_DIR"

unset NAGA_WAIT_FOR_DEBUGGER
export NAGA_WAIT_FOR_DEBUGGER

run_example() {
  python3 "$1" >"$NAGA_EXAMPLES_RESULTS_DIR/$1.txt" 2>&1
}

run_example circle.py
run_example console.py
run_example global.py
run_example meaning.py
run_example simple.py

echo "Diffing results with expected results..."
diff -r "$NAGA_EXAMPLES_EXPECTATIONS_DIR" "$NAGA_EXAMPLES_RESULTS_DIR"
# shellcheck disable=SC2181
if [[ "$?" -eq 0 ]]; then
  echo "All OK"
fi
