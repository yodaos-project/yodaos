#!/bin/sh

if [[ ! -f "/data/system/wpa_supplicant.conf" ]] || [[ ! -s "/data/system/wpa_supplicant.conf" ]]; then
    mkdir -p /data/system/
    cp /etc/wpa_supplicant.conf /data/system/
    sync
fi

if [ -f "/data/wpa.log" ]; then
    rm /data/wpa.log
fi

if [[ ! -f "/data/system/dhcpcd.conf" ]] || [[ ! -s "/data/system/dhcpcd.conf" ]]; then
    cp /etc/dhcpcd.conf /data/system/
    sync
fi

