#!/bin/bash

ifconfig wlan0 up
wpa_supplicant -iwlan0 -Dnl80211 -c/etc/wpa_supplicant.conf &
udhcpc -i wlan0
echo "nameserver 223.5.5.5" > /etc/resolv.conf
