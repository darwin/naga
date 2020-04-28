#!/usr/bin/env bash

set -e -o pipefail

# shellcheck source=_config.sh
source "$(dirname "${BASH_SOURCE[0]}")/_config.sh"

if [[ -n "$GITHUB_WORKFLOW" ]]; then
  # https://medium.com/developer-space/how-to-change-docker-data-folder-configuration-33d372669056
  # shellcheck disable=SC2016
  SERVICE_CONFIG="/lib/systemd/system/docker.service"
  set -x
  sudo ps aux | grep -i docker
  sudo systemctl stop docker
  sudo ps aux | grep -i docker
  CACHED_DOCKER_HOME=~/docker
  sudo sed -i "s#-H fd://#-H fd:// -g $CACHED_DOCKER_HOME#g" "$SERVICE_CONFIG"

  mkdir -p "$CACHED_DOCKER_HOME"
  cat <<EOF | sudo tee "/etc/docker/daemon.json"
{
 "data-root": "$CACHED_DOCKER_HOME"
}
EOF
  cat /etc/docker/daemon.json || :
  cat "$SERVICE_CONFIG" || :

#  chmod -R root:root "$CACHED_DOCKER_HOME"

  sudo systemctl daemon-reload
  sudo systemctl start docker
  sudo ps aux | grep -i docker
fi

set -x
cd "$ROOT_DIR/docker/builder"

./do.sh build

./do.sh run build

if [[ -n "$GITHUB_WORKFLOW" ]]; then
  # hack tar to always run as root
  # this is needed for cache task to properly serialize docker files
  set -x
  ls -la /bin/tar
  sudo chown root /bin/tar
  sudo chmod a+s /bin/tar
  ls -la /bin/tar
fi

./do.sh run test
