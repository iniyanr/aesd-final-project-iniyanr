#ifndef SECWRAPFS_IOCTL_H
#define SECWRAPFS_IOCTL_H

#include <linux/ioctl.h>

#ifndef __KERNEL__
#include <stdint.h>
#else
#include <linux/types.h>
#endif

struct secwrapfs_transaction {
    uint64_t inode_num;
    char operation_type[8];
    int64_t tv_sec;
    int64_t tv_nsec;
    uint64_t byte_count;
} __attribute__((packed)); /* FORCE PACKED DATA OVER ALIGNMENT GAPS */

struct secwrapfs_telemetry {
    uint64_t total_reads;
    uint64_t total_writes;
    uint32_t log_verbosity;
    struct secwrapfs_transaction last_tx;
} __attribute__((packed)); /* FORCE PACKED DATA OVER ALIGNMENT GAPS */

#define SECWRAPFS_MAGIC 's'
#define SECWRAPFS_SET_VERBOSITY _IOW(SECWRAPFS_MAGIC, 1, uint32_t)
#define SECWRAPFS_GET_TELEMETRY  _IOR(SECWRAPFS_MAGIC, 2, struct secwrapfs_telemetry)

#endif
