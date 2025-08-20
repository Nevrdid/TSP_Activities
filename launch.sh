#!/bin/sh
echo $0 $*

export PATH="$PATH:/mnt/SDCARD/System/bin"

cd "$(dirname "$0")"

/mnt/SDCARD/System/bin/activities gui -D
