#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>

#include "accord-le-ioctl.h"

#define NO    "NO"
#define SHOW  "SHOW"
#define READ  "READ"
#define RW    "RW"

void usage () {
    fprintf (stdout, "\
acle-set-sdc-mode usage:\n\
    ./acle-set-sdc-mode -m <param>\n\
     - where <param> is:\n\
          NO (nothing)\n\
          SHOW (just show reader)\n\
          READ (read mode)\n\
          RW (r/w mode)\n");
    return;
}

int main (int argc, char *argv[]) {
    
    int arg_mode = 0;

    if (argc < 2) {
        usage ();
        exit (0);
    }

    struct option options[] =
        {
            {"help", no_argument, NULL, 'h'},
            {"mode", required_argument, NULL, 'm'},
            {NULL},
        };
    while (1)
        {
            int c = getopt_long(argc, argv, "hm:", options, NULL);
            if (c == -1)
                {
                    break;
                }
            switch (c) {
                case 'h':
                    usage ();
                    exit (0);
                case 'm':
                    if (!strcmp (optarg, NO)) {
                        arg_mode = 0;
                    }
                    else if (!strcmp (optarg, SHOW)) {
                        arg_mode = 1;
                    }
                    else if (!strcmp (optarg, READ)) {
                        arg_mode = 3;
                    }
                    else if (!strcmp (optarg, RW)) {
                        arg_mode = 7;
                    }
                    else {
                        fprintf (stderr, "Unknown SD control mode\n");
                        exit (1);
                    }
                default:
                    break;
            }
        }

    int dev_fd   = 0;
    int res      = 0;

    dev_fd = open("/dev/accord-le", O_RDWR);
    if(dev_fd == -1)
    	{
            fprintf(stderr, "Error: failed to open device: %m\n");
            goto out;
        }

    res = ioctl(dev_fd, ACLE_CMD_SET_SDC, arg_mode);

    if(res)
        {
            fprintf(stderr, "ioctl failed: %m\n");
            goto out;
        }

out:
    if (dev_fd != 0 && dev_fd != -1) {
        close (dev_fd);
    }
    return res;

}
