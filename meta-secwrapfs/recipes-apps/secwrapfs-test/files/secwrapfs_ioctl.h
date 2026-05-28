#ifndef SECWRAPFS_IOCTL_H
#define SECWRAPFS_IOCTL_H

#include <linux/ioctl.h>

#ifndef __KERNEL__
#include <stdint.h>
#endif

struct secwrapfs_telemetry {
    uint64_t total_reads;
    uint64_t total_writes;
    uint32_t log_verbosity;
};

#define SECWRAPFS_MAGIC 's'

#define SECWRAPFS_SET_VERBOSITY _IOW(SECWRAPFS_MAGIC, 1, uint32_t)
#define SECWRAPFS_GET_TELEMETRY  _IOR(SECWRAPFS_MAGIC, 2, struct secwrapfs_telemetry)

#endif
