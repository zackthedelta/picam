#!/bin/sh

/usr/sbin/rfkill unblock all
sleep 1
#/usr/sbin/wpa_passphrase XXXXXXXX XXXXXXXX > /etc/wpa_supplicant.conf
#sleep 1
/usr/sbin/wpa_supplicant -B -i wlan0 -c /etc/wpa_supplicant.conf
sleep 1
/usr/sbin/udhcpc -i wlan0