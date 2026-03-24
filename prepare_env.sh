#!/bin/sh

sudo apt update

sudo apt install -y \
gawk wget git diffstat unzip texinfo gcc build-essential \
chrpath socat cpio python3 python3-pip python3-pexpect \
xz-utils debianutils iputils-ping python3-git \
python3-jinja2 libegl1-mesa libsdl1.2-dev pylint \
xterm mesa-common-dev zstd liblz4-tool \
file locales 

sudo locale-gen en_US.UTF-8
