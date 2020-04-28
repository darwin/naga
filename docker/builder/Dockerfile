FROM ubuntu:eoan

ENV DEBIAN_FRONTEND=noninteractive

# install basic deps
RUN apt-get update && apt-get install -y --no-install-recommends \
    software-properties-common wget curl git sudo fish tree gpg-agent diffutils

# installing Python is like opening a can of worms
# https://stackoverflow.com/a/59596814/84283
RUN apt-get update && apt-get install -y --no-install-recommends \
    python python3 python3-dev python3-venv

# install some tools expected by gclient or python build dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    tar xz-utils pkg-config

COPY "fs/*.sh" "/"

VOLUME ["/naga"]

ENTRYPOINT ["/builder.sh"]

# we should have both python -> 2.7.x and python3 -> 3.7.x installed
RUN "/stats.sh"