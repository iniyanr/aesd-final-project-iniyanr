SUMMARY = "User-space control utility for SecWrapFS"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/GPL-2.0-only;md5=801f80980d171dd6425610833a22dbe6"

SRC_URI = "file://secwrapctl.c"

S = "${WORKDIR}"

do_compile() {
    ${CC} ${CFLAGS} ${LDFLAGS} secwrapctl.c -o secwrapctl
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 secwrapctl ${D}${bindir}
}
