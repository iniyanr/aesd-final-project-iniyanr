SUMMARY = "SecWrapFS Multithreaded Background Transaction Daemon"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

DEPENDS += "systemd"

SRC_URI = " \
    file://secwrapd.c \
    file://secwrapfs_ioctl.h \
    file://org.secwrapfs.conf \
"

S = "${WORKDIR}"

do_compile() {
    ${CC} ${CFLAGS} ${LDFLAGS} secwrapd.c -o secwrapd -lpthread -lsystemd
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 secwrapd ${D}${bindir}/

    # Install the D-Bus system policy configuration
    install -d ${D}${datadir}/dbus-1/system.d
    install -m 0644 ${WORKDIR}/org.secwrapfs.conf ${D}${datadir}/dbus-1/system.d/
}

FILES:${PN} += "${datadir}/dbus-1/system.d/org.secwrapfs.conf"
