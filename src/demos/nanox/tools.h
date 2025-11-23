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

#endif /* TOOLS_H */

