#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <stddef.h>

#include <fcntl.h>
#include <string.h>

#include "accord-le-ioctl.h"

int main(int argc, char *argv[])
{
    const char *help = "\
Print an attached tmid as-is\n\
Usage:\n\
    acle-get-tmid\n\
Options\n\
    -h,--help,help - show this message\n\
    -s,--short     - print simpler output\n";
    int dev_fd = 0;
    int result = 0;
    int is_short = 0;
    struct acle_tmid tmid;

    memset(&tmid, 0, sizeof(tmid));
    int i = 1;
    for(i = 1; i < argc; i++)
        {
            if(!strcmp("-h", argv[i]) || !strcmp("--help", argv[i]) || !strcmp("help", argv[i]))
                {
                    fprintf(stdout, help);
                    exit(0);
                }
            if(!strcmp("-s", argv[i]) || !strcmp("--short", argv[i]))
                {
                    is_short = 1;
                }
        }

    dev_fd = open("/dev/accord-le", O_RDWR);
    if(dev_fd == -1)
        {
            if (!is_short)
                {
                    fprintf(stderr, "failed to open device: %m\n");
                }
            exit(1);
        }

    result = ioctl(dev_fd, ACLE_CMD_TM_GET_ID, &tmid);
    if(result)
        {
            if(!is_short)
            {
                fprintf(stderr,
                        "failed call ioctl command: ret=%d(%s) errno=%m\n",
                        result, strerror(result));
                fprintf(stderr,
                        "consider checking the tm is attached\n");
            }
            exit(1);
        }
    else
        {
            if(!is_short)
                {
                    fprintf(stdout, "\
Got tm:\n\
    %02x %02x%02x%02x%02x%02x%02x %02x\n",
                        tmid.type,
                        tmid.sn[0], tmid.sn[1], tmid.sn[2],
                        tmid.sn[3], tmid.sn[4], tmid.sn[5],
                        tmid.crc
                    );
                }
            else
                {
                    fprintf(stdout, "\
%02x%02x%02x%02x%02x%02x%02x%02x",
                        tmid.type,
                        tmid.sn[0], tmid.sn[1], tmid.sn[2],
                        tmid.sn[3], tmid.sn[4], tmid.sn[5],
                        tmid.crc
                    );
                }
        }

    close(dev_fd);
    return 0;
}
