SUMMARY = "Helper for waiting until DHCP assigns a usable IPv4 address"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://picam-wait-network"

S = "${WORKDIR}"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${WORKDIR}/picam-wait-network ${D}${bindir}/picam-wait-network
}

RDEPENDS:${PN} = "iproute2"
