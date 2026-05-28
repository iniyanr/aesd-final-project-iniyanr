#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include "secwrapfs_ioctl.h"

#define TEST_FILE "/mnt/secwrap/control_node.txt"

void run_concurrent_worker(int id) {
    int fd = open(TEST_FILE, O_RDWR | O_CREAT | O_APPEND, 0666);
    if (fd < 0) {
        perror("Worker open failed");
        exit(1);
    }
    
    write(fd, "Data\n", 5);
    char buffer[10];
    lseek(fd, 0, SEEK_SET);
    read(fd, buffer, 5);
    
    close(fd);
    exit(0);
}

int main() {
    int fd;
    uint32_t verbosity;
    struct secwrapfs_telemetry metrics;

    fd = open(TEST_FILE, O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        perror("Failed to look up target control node file descriptor");
        return 1;
    }

    printf("--- Phase 1: Verify IOCTL Control parameter changes ---\n");
    verbosity = 0; // Turn off tracing
    if (ioctl(fd, SECWRAPFS_SET_VERBOSITY, &verbosity) < 0) {
        perror("IOCTL Set Verbosity failed");
        close(fd);
        return 1;
    }
    printf("[PASS] Successfully set log verbosity to 0 (Silent Mode)\n");

    printf("\n--- Phase 2: Stress Concurrency Engine with Parallel Operations ---\n");
    for (int i = 0; i < 5; i++) {
        if (fork() == 0) {
            run_concurrent_worker(i);
        }
    }

    while (wait(NULL) > 0);
    printf("[PASS] All asynchronous processing threads finished.\n");

    printf("\n--- Phase 3: Telemetry Counter Validation ---\n");
    if (ioctl(fd, SECWRAPFS_GET_TELEMETRY, &metrics) < 0) {
        perror("IOCTL Fetch Telemetry failed");
        close(fd);
        return 1;
    }

    printf("Telemetry readout results:\n");
    printf(" -> System Read Ops Intercepted: %llu\n", metrics.total_reads);
    printf(" -> System Write Ops Intercepted: %llu\n", metrics.total_writes);
    printf(" -> Runtime Tracing Verbosity Setting: %u\n", metrics.log_verbosity);

    close(fd);
    return 0;
}
