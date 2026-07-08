SUMMARY = "Boot-time Wi-Fi provisioning for the Pi camera image"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://picam-rfkill-unblock.service \
    file://picam-wifi.config \
"

S = "${WORKDIR}"

inherit systemd

do_install() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/picam-rfkill-unblock.service ${D}${systemd_system_unitdir}/picam-rfkill-unblock.service

    install -d ${D}${localstatedir}/lib/connman
    install -m 0600 ${WORKDIR}/picam-wifi.config ${D}${localstatedir}/lib/connman/picam-wifi.config
}

SYSTEMD_SERVICE:${PN} = "picam-rfkill-unblock.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

FILES:${PN} += " \
    ${localstatedir}/lib/connman/picam-wifi.config \
"

RDEPENDS:${PN} = " \
    connman \
    util-linux-rfkill \
    wpa-supplicant \
"
