SUMMARY = "RTSP server for the shared Pi camera RTP fanout"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://picam-rtsp.c \
    file://picam-rtsp.service \
    file://picam-rtsp.default \
"

S = "${WORKDIR}"

DEPENDS = "gstreamer1.0 gstreamer1.0-rtsp-server"

inherit pkgconfig systemd

do_compile() {
    ${CC} ${CFLAGS} ${LDFLAGS} \
        ${WORKDIR}/picam-rtsp.c \
        -o ${B}/picam-rtsp \
        $(pkg-config --cflags --libs gstreamer-1.0 gstreamer-rtsp-server-1.0)
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${B}/picam-rtsp ${D}${bindir}/picam-rtsp

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/picam-rtsp.service ${D}${systemd_system_unitdir}/picam-rtsp.service

    install -d ${D}${sysconfdir}/default
    install -m 0644 ${WORKDIR}/picam-rtsp.default ${D}${sysconfdir}/default/picam-rtsp
}

SYSTEMD_SERVICE:${PN} = "picam-rtsp.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

RDEPENDS:${PN} = " \
    gstreamer1.0 \
    gstreamer1.0-plugins-good-rtp \
    gstreamer1.0-plugins-good-udp \
    gstreamer1.0-plugins-bad-videoparsersbad \
    picam-mjpeg \
    picam-network-wait \
"
