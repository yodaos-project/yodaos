#!/bin/bash

# Make sure only root can run our script
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root." 1>&2
   exit 1
fi

echo "This script has been tested on Ubuntu (14.04 LTS)."
echo "It may have to be updated for other Linux distributions/versions."

backup_dir="/tmp/bluez-driver-backup/$(uname -r)/"
driver_dir="/lib/modules/$(uname -r)/kernel/drivers"
brcm_udev_file="/etc/udev/rules.d/50-brcm.rules"

# Check if BlueZ drivers installed
if [[ -d $driver_dir/bluetooth ]] ; then
    echo "BlueZ drivers present on your platform.";
    echo "BSA/btusb is not compatible with blueZ.";
    read -p "Do you want to remove (and backup) them (y/N)?" yn;
    case $yn in
        [Yy]*)
            rm -rf $backup_dir
            mkdir -p $backup_dir
            echo "Moving" $driver_dir/bluetooth/ in $backup_dir
            mv $driver_dir/bluetooth/ $backup_dir
            ;;
        *)
            echo "BlueZ not removed. BSA/btusb may not work!";
            ;;
    esac
fi

# by default, depmod not needed
depmod_needed=0

# Create brcm driver folder if needed
if [ ! -d $driver_dir/brcm ] ; then
    mkdir $driver_dir/brcm
    depmod_needed=1
fi

# Check if udev rule file exists for btusb
if [ ! -e $brcm_udev_file ] ; then
    touch $brcm_udev_file;
    echo "# udev rule for btusb" > $brcm_udev_file
    echo "KERNEL==\"btusb*\", MODE=\"0666\"" >> $brcm_udev_file
    depmod_needed=1
fi

# Copy btusb driver in Kernel
echo "Installing btusb.ko in" $driver_dir/brcm
cp -v btusb.ko $driver_dir/brcm/btusb.ko

# If driver scan needed
if [ $depmod_needed == 1 ] ; then
    echo "Refreshing Linux driver list..."
    depmod -a;
    echo "BTUSB driver installed. You need to reboot your computer."
fi

