SUMMARY = "CPU and memory usage monitoring for the Pi camera image"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://picam-monitor.sh \
    file://picam-monitor.service \
"

S = "${WORKDIR}"

inherit systemd

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${WORKDIR}/picam-monitor.sh ${D}${bindir}/picam-monitor.sh

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/picam-monitor.service ${D}${systemd_system_unitdir}/picam-monitor.service
}

SYSTEMD_SERVICE:${PN} = "picam-monitor.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

RDEPENDS:${PN} = "busybox"
