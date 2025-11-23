#ifndef TOOLS_H
#define TOOLS_H

#include <linuxmt/mem.h>
#include <linuxmt/sched.h>
#include <linuxmt/mm.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>

/* -------------------------- INTERNAL HELPERS -------------------------- */

static int read_task_entry(int fd, unsigned int off, unsigned int ds,
                           struct task_struct *task)
{
    off_t addr = ((off_t)ds << 4) + off;

    if (lseek(fd, addr, SEEK_SET) != addr)
        return -1;

    if (read(fd, task, sizeof(struct task_struct))
            != sizeof(struct task_struct))
        return -1;

    return 0;
}

static int get_process_name(int fd, unsigned int seg,
                            unsigned int off, char *buf, int buflen)
{
    unsigned int strptr;
    off_t addr = ((off_t)seg << 4) + off;

    if (lseek(fd, addr, SEEK_SET) != addr)
        return -1;

    if (read(fd, &strptr, 2) != 2)
        return -1;

    addr = ((off_t)seg << 4) + (off_t)strptr;

    if (lseek(fd, addr, SEEK_SET) != addr)
        return -1;

    if (read(fd, buf, buflen) != buflen)
        return -1;

    buf[buflen - 1] = 0;
    return 0;
}

/* -------------------------- PUBLIC FUNCTIONS -------------------------- */

/* Search for process by name */
static int get_pid(const char *name)
{
    int fd;
    unsigned int ds, off, j;
    struct task_struct tt;
    char pname[64];

    fd = open("/dev/kmem", O_RDONLY);
    if (fd < 0)
        return -1;

    if (ioctl(fd, MEM_GETDS, &ds) < 0) {
        close(fd);
        return -1;
    }
    if (ioctl(fd, MEM_GETTASK, &off) < 0) {
        close(fd);
        return -1;
    }

    for (j = off;; j += sizeof(struct task_struct))
    {
        if (read_task_entry(fd, j, ds, &tt) < 0)
            break;

        if (tt.t_kstackm != KSTACK_MAGIC)
            break;
        if (tt.state == TASK_UNUSED)
            continue;
        if (tt.mm.flags & DS_SWAP)
            continue;

        if (get_process_name(fd, tt.t_regs.ss,
                             tt.t_begstack + 2,
                             pname, sizeof(pname)) == 0)
        {
            if (!strcmp(name, pname)) {
                close(fd);
                return tt.pid;
            }
        }
    }

    close(fd);
    return -1;
}

/* Return free and total conventional memory */
static int get_free_memory(unsigned long *free_bytes,
                           unsigned long *total_bytes)
{
    int fd;
    struct mmap info;

    fd = open("/dev/kmem", O_RDONLY);
    if (fd < 0)
        return -1;

    if (ioctl(fd, MEM_GETMMAP, &info) < 0) {
        close(fd);
        return -1;
    }

    close(fd);

    if (free_bytes)
        *free_bytes = (unsigned long)info.free * 16;
    if (total_bytes)
        *total_bytes = (unsigned long)info.total * 16;

    return 0;
}

/* Kill a process */
static int force_close(int pid)
{
    if (pid <= 0)
        return -1;

    if (kill(pid, SIGKILL) < 0)
        return -1;

    return 0;
}

#endif

