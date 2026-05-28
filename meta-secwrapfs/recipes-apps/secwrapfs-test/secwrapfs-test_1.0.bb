SUMMARY = "User-space concurrency and IOCTL testing app for SecWrapFS"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://secwrapfs-test.c \
    file://secwrapfs_ioctl.h \
"

S = "${WORKDIR}"

# Tell BitBake how to compile this using the target cross-compiler
do_compile() {
    ${CC} ${CFLAGS} ${LDFLAGS} secwrapfs-test.c -o secwrapfs-test
}

# Install the binary into /usr/bin/ inside the QEMU rootfs image
do_install() {
    install -d ${D}${bindir}
    install -m 0755 secwrapfs-test ${D}${bindir}/
}
