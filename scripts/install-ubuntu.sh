#!/usr/bin/env bash

set -e -o pipefail
# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/_config.sh"

cd "$ROOT_DIR"

echo_err "this no longer works"
echo_info "follow the readme"

exit 1

sudo rm /usr/bin/python
sudo ln -s /usr/bin/python2 /usr/bin/python

python setup.py v8

sudo rm /usr/bin/python
sudo ln -s /usr/bin/python3 /usr/bin/python

python setup.py stpyv8
sudo python setup.py install
