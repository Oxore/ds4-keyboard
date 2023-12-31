#!/bin/bash

# Program will just exit on any failed check (an expression in square brackets) due to -e flag.
set -e

devpath=$1
program=$2
[ -n "$devpath" ]
[ -n "$program" ]

event_name=$(basename $devpath)
is_event=
case "$event_name" in
    event*) is_event="yes" ;;
    *);;
esac
[ "$is_event" = "yes" ]
devpath="/sys$devpath"

# A bunch of heuristic checks to determine that it is actually the controller
# and neither a touchpad nor motion sensors which all have an event input
# device and share the same udev attributes.
cap_path=$devpath/device/capabilities
[ -f $cap_path/abs ]
[ -f $cap_path/ev ]
[ -f $cap_path/ff ]
[ -f $cap_path/key ]
[ -f $cap_path/led ]
[ -f $cap_path/msc ]
[ -f $cap_path/rel ]
[ -f $cap_path/snd ]
[ -f $cap_path/sw ]
[ "$(cat $cap_path/abs)" = "3003f" ]
[ "$(cat $cap_path/ev)" = "1b" ]
[ "$(cat $cap_path/ff)" = "0" ]
[ "$(cat $cap_path/key)" = "7fdb000000000000 0 0 0 0" ]
[ "$(cat $cap_path/led)" = "0" ]
[ "$(cat $cap_path/msc)" = "10" ]
[ "$(cat $cap_path/rel)" = "0" ]
[ "$(cat $cap_path/snd)" = "0" ]
[ "$(cat $cap_path/sw)" = "0" ]

$program /dev/input/$event_name
