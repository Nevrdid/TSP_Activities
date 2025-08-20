#!/bin/sh
echo $0 $*

export PATH="$PATH:/mnt/SDCARD/System/bin"

cd "$(dirname "$0")"

skins="$(jq -r '.["theme"]' /mnt/UDISK/system.json)"
if [ "$skins" = "../res/" ] || [ "$skins" = "null" ]; then
  skins="CrossMix - OS"
else
   skins="${skins##*/Themes/}"
fi
backgrounds="$(jq -r '.["BACKGROUNDS"]' /mnt/SDCARD/System/etc/crossmix.json)"
echo "[Themes]
skins_theme=$skins
backgrounds_theme=$backgrounds
" > data/config.ini

/mnt/SDCARD/System/bin/activities gui -D
