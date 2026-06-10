#include <stdio.h>
#include <stdlib.h>
#include <systemd/sd-bus.h>

int main() {
    sd_bus *bus = NULL;
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *reply = NULL;
    uint64_t reads, writes, time_sec, inode;
    int r;

    /* Initialize connection to the System D-Bus */
    r = sd_bus_default_system(&bus);
    if (r < 0) {
        fprintf(stderr, "Failed to connect to system bus: %s\n", strerror(-r));
        return 1;
    }

    /* Synchronously call the background daemon's D-Bus interface */
    r = sd_bus_call_method(bus,
                           "org.secwrapfs.Daemon",           /* Service target */
                           "/org/secwrapfs/Control",          /* Object path */
                           "org.secwrapfs.Control",           /* Interface */
                           "GetTelemetry",                    /* Method name */
                           &error, &reply, "");               /* Input parameters */
    if (r < 0) {
        fprintf(stderr, "IPC Transaction Failed: %s\n", error.message);
        sd_bus_error_free(&error);
        return 1;
    }

    /* Parse response format string payload types */
    
	sd_bus_message_read(reply, "tttt", &reads, &writes, &inode, &time_sec);

	printf("====== secwrapctl execution monitor ======\n");
	printf("Total Intercepted Reads: %lu\n", reads);
	printf("Total Intercepted Writes: %lu\n", writes);
	printf("Last Tx Inode Reference: %lu\n", inode);
	printf("Last Precision Access Epoch: %lu seconds\n", time_sec);
    sd_bus_message_unref(reply);
    sd_bus_close(bus);
    return 0;
}
