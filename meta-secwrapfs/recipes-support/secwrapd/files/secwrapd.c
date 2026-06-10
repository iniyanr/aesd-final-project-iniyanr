#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <systemd/sd-bus.h> /* Standard lightweight Yocto D-Bus framework */
#include "secwrapfs_ioctl.h"

#define MOUNTED_NODE "/mnt/secwrap/vault.txt"

static sd_bus *bus = NULL;
pthread_mutex_t daemon_mutex = PTHREAD_MUTEX_INITIALIZER;
struct secwrapfs_telemetry shared_cache;

/* Worker thread polling the driver transaction data metrics continuously */
void *listener_thread_worker(void *arg) {
    while (1) {
        int fd = open(MOUNTED_NODE, O_RDONLY);
        if (fd >= 0) {
            pthread_mutex_lock(&daemon_mutex);
            ioctl(fd, SECWRAPFS_GET_TELEMETRY, &shared_cache);
            pthread_mutex_unlock(&daemon_mutex);
            close(fd);
        }
        usleep(500000); // Polling context interval
    }
    return NULL;
}

/* D-Bus IPC callback to deliver telemetry cleanly over the system framework */
static int method_get_telemetry(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
    pthread_mutex_lock(&daemon_mutex);
    int r = sd_bus_reply_method_return(m, "tttt", 
        shared_cache.total_reads, 
        shared_cache.total_writes,
        shared_cache.last_tx.tv_sec,
        shared_cache.last_tx.inode_num);
    pthread_mutex_unlock(&daemon_mutex);
    return r;
}

/* Expose the interface onto the system bus topology */
/* Expose the interface onto the system bus topology */
static const sd_bus_vtable daemon_vtable[] = {
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD("GetTelemetry", "", "tttt", method_get_telemetry, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_VTABLE_END
};

int main() {
    pthread_t thread_id;
    
    // Connect to the D-Bus system bus topology
    sd_bus_default_system(&bus);
    sd_bus_add_object_vtable(bus, NULL, "/org/secwrapfs/Control", "org.secwrapfs.Control", daemon_vtable, NULL);
    sd_bus_request_name(bus, "org.secwrapfs.Daemon", 0);

    // Launch worker listener execution utility thread
    pthread_create(&thread_id, NULL, listener_thread_worker, NULL);
    
    printf("SecWrapFS multi-threaded daemon active and listening over D-Bus...\n");
    while (1) {
        sd_bus_process(bus, NULL);
        sd_bus_wait(bus, (uint64_t)-1);
    }
    return 0;
}
