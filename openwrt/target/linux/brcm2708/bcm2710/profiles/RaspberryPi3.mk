#
# Copyright (C) 2016 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/RaspberryPi_3
  NAME:=Raspberry Pi 3 Model B
  PACKAGES:=brcmfmac43430-firmware-sdio kmod-brcmfmac wpad-mini
endef
define Profile/RaspberryPi_3/Description
  Raspberry Pi 3 Model B
endef
$(eval $(call Profile,RaspberryPi_3))


define Profile/RaspberryPi_3B_Plus
  NAME:=Raspberry Pi 3 Model B+
PACKAGES:=brcmfmac-firmware-43430-sdio brcmfmac-board-rpi2 brcmfmac-firmware-43455-sdio brcmfmac-board-rpi3 kmod-brcmfmac wpad-basic
endef
define Profile/RaspberryPi_3B_Plus/Description
  Raspberry Pi 3 Model B+
endef
$(eval $(call Profile,RaspberryPi_3B_Plus))
