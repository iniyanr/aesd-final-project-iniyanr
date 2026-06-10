SUMMARY = "SecWrapFS IPC Command Line Utility"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

DEPENDS += "systemd"

SRC_URI = " \
    file://secwrapctl.c \
    file://secwrapfs_ioctl.h \
"

S = "${WORKDIR}"

do_compile() {
    ${CC} ${CFLAGS} ${LDFLAGS} secwrapctl.c -o secwrapctl -lsystemd
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 secwrapctl ${D}${bindir}/
}
