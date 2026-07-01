#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <stddef.h>

#include <fcntl.h>
#include <string.h>

#include "accord-le-ioctl.h"

void usage(const char *argv0)
{
    fprintf(stderr, "%s - logout\n", argv0);
    fprintf(stderr, "Usage: %s [OPTIONS]\n", argv0);
    fprintf(stderr, "OPTIONS:\n");
    fprintf(stderr, "\t-h, --help       print this message\n");
}

int main(int argc, char *argv[])
{
    int dev_fd = 0;
    int result = 0;

    int i = 1;
    for(i = 1; i < argc; i++)
        {
            if(!strcmp("-h", argv[i]) || !strcmp("--help", argv[i]) || !strcmp("help", argv[i]))
                {
                    usage(argv[0]);
                    exit(0);
                }
        }

    dev_fd = open("/dev/accord-le", O_RDWR);
    if (dev_fd == -1)
        {
            fprintf(stderr, "failed to open device: %m\n");
            exit(1);
        }

    result = ioctl(dev_fd, ACLE_CMD_LOGOUT, NULL);
    if (result)
        {
            fprintf(stderr,
                    "failed call ioctl command: ret=%d(%s) errno=%m\n",
                    result, strerror(result));
            exit(1);
        }
    else
        {
            fprintf(stdout, "Logout OK\n");
        }

    close(dev_fd);
    return 0;
}
