#!/bin/bash

# rerun the script under sudo if not ran under root
(( EUID != 0 )) && exec sudo -- "$0" "$@"

yum install -y \
  gstreamer1 \
  gstreamer1-devel
