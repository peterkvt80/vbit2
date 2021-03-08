#!/bin/bash
# run vbit2 and raspi-teletext using variables read from config.sh

CONFIG=$HOME/.teletext-services/config

if [ -f "$CONFIG" ]; then
  source $CONFIG
else
  echo no config found, please run vbit-config
  exit 1
fi

if [ -z "$SELECTED" ]; then
  echo no service selected, please run select_service.sh
  exit 1
fi

for i in ${!INSTALLED[@]}; do
  IFS=',' read -r -a INSTALLEDARRAY <<< "${INSTALLED[i]}"
  if [ "${INSTALLEDARRAY[0]}" == "$SELECTED" ]; then
    PAGESDIRECTORY="${INSTALLEDARRAY[1]}"
    break
  fi
done

if [ ! -d "$PAGESDIRECTORY" ]; then
  echo pages directory is missing
  exit 1
fi

trap 'sudo $HOME/raspi-teletext/tvctl off; kill -- -$$' EXIT

sudo $HOME/raspi-teletext/tvctl on

# find first uncommented lines_per_field in config file
lpf=`grep -m 1 ^lines_per_field "$PAGESDIRECTORY/vbit.conf" | cut -f2- -d= | tr -d '\r'`
num=0 # default to 16 lines
if [ ! -z "$lpf" ]; then
  if [ "$lpf" -ge 1 ] && [ "$lpf" -le 15 ]; then
    num=65535
    for ((i=0;i<lpf;i++)); do
      num=$((num>>1))
    done
  fi
fi
MASK=`printf "0x%04x" $num`

$HOME/raspi-teletext/teletext -m $MASK - < <( $HOME/.local/bin/vbit2 --dir "$PAGESDIRECTORY" )
