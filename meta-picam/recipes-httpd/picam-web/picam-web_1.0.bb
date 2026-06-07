SUMMARY = "Static web GUI for the Pi camera RTSP service"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://index.html \
    file://styles.css \
    file://app.js \
"

S = "${WORKDIR}"

do_install() {
    install -d ${D}${localstatedir}/www/picam
    install -m 0644 ${WORKDIR}/index.html ${D}${localstatedir}/www/picam/index.html
    install -m 0644 ${WORKDIR}/styles.css ${D}${localstatedir}/www/picam/styles.css
    install -m 0644 ${WORKDIR}/app.js ${D}${localstatedir}/www/picam/app.js
}

FILES:${PN} = "${localstatedir}/www/picam"

RDEPENDS:${PN} = "nginx picam-rtsp"
