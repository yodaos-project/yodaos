# Telephony packages feed

## Description

This is an OpenWrt package feed containing community maintained telephony packages.

## Usage

To use these packages, add the following line to the feeds.conf
in the OpenWrt buildroot:

```
src-git telephony https://github.com/openwrt/telephony.git
```

This feed should be included and enabled by default in the OpenWrt buildroot. To install all its package definitions, run:

```
./scripts/feeds update telephony
./scripts/feeds install -a -p telephony
```

The telephony packages should now appear in menuconfig.
