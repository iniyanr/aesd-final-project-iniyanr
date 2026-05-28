SUMMARY = "Recipe for out-of-tree SecWrapFS kernel module"
SECTION = "kernel"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/GPL-2.0-only;md5=801f80980d171dd6425610833a22dbe6"
PV = "2.0"
inherit module

SRC_URI = "file://secwrapfs.c \
           file://secwrapfs_ioctl.h \ 
	   file://Makefile \
          "

S = "${WORKDIR}"

RPROVIDES:${PN} += "kernel-module-secwrapfs"

# Create a configuration file to force-load the module at boot
do_install:append() {
    install -d ${D}${sysconfdir}/modules-load.d
    echo "secwrapfs" > ${D}${sysconfdir}/modules-load.d/secwrapfs.conf
}

# Ensure the configuration file is included in the runtime package files
FILES:${PN} += "${sysconfdir}/modules-load.d/secwrapfs.conf"
