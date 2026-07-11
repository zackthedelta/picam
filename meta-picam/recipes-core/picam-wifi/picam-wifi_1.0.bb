SUMMARY = "Boot-time Wi-Fi provisioning for the Pi camera image"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://picam-wifi.service \
    file://picam-wifi.sh \
"

S = "${WORKDIR}"

inherit systemd

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${B}/picam-wifi.sh ${D}${bindir}/picam-wifi.sh

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/picam-wifi.service ${D}${systemd_system_unitdir}/picam-wifi.service
}

SYSTEMD_SERVICE:${PN} = "picam-wifi.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

RDEPENDS:${PN} = " \
    connman \
    util-linux-rfkill \
    wpa-supplicant \
"
