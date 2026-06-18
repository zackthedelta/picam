SUMMARY = "Shared Pi camera fanout for browser MJPEG and RTSP RTP"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://picam-mjpeg.c \
    file://picam-mjpeg.default \
    file://picam-mjpeg.service \
"

S = "${WORKDIR}"

DEPENDS = "gstreamer1.0 gstreamer1.0-plugins-base gstreamer1.0-plugins-good libcamera"

inherit pkgconfig systemd

# Runtime GStreamer plugin splits are pulled in explicitly below.
INSANE_SKIP:${PN} += "build-deps"

do_compile() {
    ${CC} ${CFLAGS} ${LDFLAGS} \
        ${WORKDIR}/picam-mjpeg.c \
        -o ${B}/picam-mjpeg \
        $(pkg-config --cflags --libs gstreamer-1.0 gstreamer-app-1.0)
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${B}/picam-mjpeg ${D}${bindir}/picam-mjpeg

    install -d ${D}${sysconfdir}/default
    install -m 0644 ${WORKDIR}/picam-mjpeg.default ${D}${sysconfdir}/default/picam-mjpeg

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/picam-mjpeg.service ${D}${systemd_system_unitdir}/picam-mjpeg.service
}

SYSTEMD_SERVICE:${PN} = "picam-mjpeg.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

RDEPENDS:${PN} = " \
    gstreamer1.0 \
    gstreamer1.0-plugins-base-videoconvertscale \
    gstreamer1.0-plugins-base-app \
    gstreamer1.0-plugins-good-jpeg \
    gstreamer1.0-plugins-good-rtp \
    gstreamer1.0-plugins-good-udp \
    gstreamer1.0-plugins-bad-videoparsersbad \
    gstreamer1.0-plugins-ugly-x264 \
    libcamera-gst \
    picam-network-wait \
"
