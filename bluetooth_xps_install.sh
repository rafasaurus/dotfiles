#!/bin/bash
git clone https://aur.archlinux.org/bcm4350-firmware.git
cd bcm4350-firmware/
sudo mv BCM4350C5_003.006.007.0095.1703.hcd /lib/firmware/brcm/BCM-0a5c-6412.hcd
cd ../
rm -rf bcm4350-firmware
