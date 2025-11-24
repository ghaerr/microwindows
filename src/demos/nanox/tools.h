#ifndef TOOLS_H
#define TOOLS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

/*
 * get_pid: returns PID of process by name
 * returns -1 if not found
 * This uses "ps" output parsing (ELKS userland only)
 */
static int get_pid(const char *name)
{
    FILE *fp;
    char line[128];
    int pid;
    char pname[64];

    fp = popen("/bin/ps", "r");
    if (!fp) return -1;

    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "%d %63s", &pid, pname) == 2) {
            if (strcmp(pname, name) == 0) {
                pclose(fp);
                return pid;
            }
        }
    }

    pclose(fp);
    return -1;
}

/*
 * force_close: sends SIGKILL to the given PID
 * returns 0 on success, -1 on failure
 */
static int force_close(int pid)
{
    if (kill(pid, SIGKILL) == 0) {
        return 0;
    } else {
        perror("force_close");
        return -1;
    }
}

/*
 * get_free_total_memory:
 * Runs `meminfo -b` and parses the "Memory usage" line.
 *
 * Returns:
 *   0 success
 *  -1 failure
 */
static int get_free_total_memory(unsigned int *kb_free, unsigned int *kb_total)
{
    FILE *fp;
    char line[160];

    if (!kb_free || !kb_total)
        return -1;

    fp = popen("meminfo -b", "r");
    if (!fp)
        return -1;

    /* Initialize outputs */
    *kb_free = 0;
    *kb_total = 0;

    while (fgets(line, sizeof(line), fp)) {

        /* Look specifically for the summary line */
        if (strncmp(line, "Memory usage", 12) == 0) {
            /*
             * Format example:
             * Memory usage  494KB total,  163KB used,  331KB free
             */
            unsigned int total = 0, used = 0, free = 0;

            if (sscanf(line,
                       "Memory usage %uKB total, %uKB used, %uKB free",
                       &total, &used, &free) == 3)
            {
                *kb_total = total;
                *kb_free = free;
                pclose(fp);
                return 0;
            }
        }
    }

    pclose(fp);
    return -1;
}

#endif /* TOOLS_H */

