#!/bin/sh

IFACE="${PICAM_WIFI_IFACE:-wlan0}"
LEASE_DIR="/var/lib/picam-wifi"
REQUESTED_IP_FILE="${LEASE_DIR}/${IFACE}.last-ip"

/usr/sbin/rfkill unblock all
sleep 1
#/usr/sbin/wpa_passphrase XXXXXXXX XXXXXXXX > /etc/wpa_supplicant.conf
#sleep 1
/usr/sbin/wpa_supplicant -B -i "${IFACE}" -c /etc/wpa_supplicant.conf
sleep 1

set -- -i "${IFACE}"

if [ -r "/sys/class/net/${IFACE}/address" ]; then
    MAC="$(tr -d ':' < "/sys/class/net/${IFACE}/address")"
    if [ -n "${MAC}" ]; then
        set -- "$@" -x "0x3d:01${MAC}"
    fi
fi

if [ -s "${REQUESTED_IP_FILE}" ]; then
    REQUESTED_IP="$(sed -n '1s/[^0-9.]//gp' "${REQUESTED_IP_FILE}")"
    if [ -n "${REQUESTED_IP}" ]; then
        set -- "$@" -r "${REQUESTED_IP}"
    fi
fi

/usr/sbin/udhcpc "$@"

CURRENT_IP="$(  "${IFACE}" | awk '
    /inet / {
        for (i = 1; i <= NF; i++) {
            if ($i == "inet") {
                print $(i + 1)
                exit
            }
            if ($i ~ /^addr:/) {
                sub(/^addr:/, "", $i)
                print $i
                exit
            }
        }
    }
')"

if [ -n "${CURRENT_IP}" ]; then
    mkdir -p "${LEASE_DIR}"
    printf '%s' "${CURRENT_IP}" > "${REQUESTED_IP_FILE}"
fi
