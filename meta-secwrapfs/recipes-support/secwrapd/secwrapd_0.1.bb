SUMMARY = "Multi-threaded auditing daemon for SecWrapFS"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/GPL-2.0-only;md5=801f80980d171dd6425610833a22dbe6"

SRC_URI = "file://secwrapd.c"

S = "${WORKDIR}"

do_compile() {
    ${CC} ${CFLAGS} ${LDFLAGS} secwrapd.c -o secwrapd -lpthread
}

do_install() {
    install -d ${D}${sbindir}
    install -m 0755 secwrapd ${D}${sbindir}
}
