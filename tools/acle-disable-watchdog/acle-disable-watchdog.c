#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <stddef.h>


//#include <sys/types.h>
//#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "accord-le-ioctl.h"

void usage(const char *argv0)
{
    fprintf(stderr, "%s - disable watchdog timer\n", argv0);
    fprintf(stderr, "Usage: %s [OPTIONS]\n", argv0);
    fprintf(stderr, "OPTIONS:\n");
    fprintf(stderr, "\t-h, --help       print this message\n");
}

int main(int argc, char *argv[])
{
    int dev_fd = 0;
    int result = 0;
    struct option options[] =
        {
            {"help",    optional_argument, NULL, 'h'},
            {NULL},
        };
    while (1)
        {
            int c = getopt_long(argc, argv, "vh:", options, NULL);
            if (c == -1)
                {
                    break;
                }
            switch (c)
                {
                    case 'h':
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

    result = ioctl(dev_fd, ACLE_DISABLE_WATCHDOG_TIMER, NULL);
    if (result)
        {
            fprintf(stderr,
                    "failed call ioctl command: ret=%d(%s) errno=%m\n",
                    result, strerror(result));
            exit(1);
        }

    close(dev_fd);
    return 0;
}
