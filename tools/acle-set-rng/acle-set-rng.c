#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <stddef.h>

#include <fcntl.h>
#include <string.h>

#include "accord-le-ioctl.h"

typedef struct {
    int use_software;
} Args;

int is_ok_prefix(const char *pattern, char *str_to_test)
{
    unsigned i = 0;
    while('\0' != pattern[i])
        {
            if (0 != i && '\0' == str_to_test[i])
                {
                    return 1;
                }
            else if (pattern[i] != str_to_test[i])
                {
                    return 0;
                }
            i++;
        }
    return '\0' == str_to_test[i];
}

void parse_args(int argc, char *argv[], Args *out_args)
{
    const char *help = "\
Set random generator mode: hardware or software\n\
Usage:\n\
    acle-set-rng COMMAND\n\
Commands:\n\
    -h,--help,help             - show this message\n\
    0,hardware (may be prefix) - use hardware to generate randoms\n\
    1,software (may be prefix) - use software to generate randoms\n";

    Args args;

    memset(&args, 0, sizeof(args));

    if(2 > argc)
        {
            fprintf(stderr, "Too few arguments\n");
            fprintf(stderr, "%s", help);
            exit(-1);
        }
    if(!strcmp("-h", argv[1])
       || !strcmp("--help", argv[1])
       || !strcmp("help", argv[1]))
        {
            fprintf(stdout, "%s", help);
            exit(0);
        }
    if (!strcmp("0", argv[1]) || is_ok_prefix("hardware", argv[1]))
        {
            args.use_software = 0;
        }
    else if (!strcmp("1", argv[1]) || is_ok_prefix("software", argv[1]))
        {
            args.use_software = 1;
        }
    else
        {
            fprintf(stderr, "Unexpected argument \"%s\"\n", help);
            fprintf(stderr, "%s", help);
            exit(-1);
        }
    *out_args = args;
}

int main(int argc, char *argv[])
{
    int dev_fd = 0;
    int result = 0;
    Args args;

    memset(&args, 0, sizeof(args));

    parse_args(argc, argv, &args);

    dev_fd = open("/dev/accord-le", O_RDWR);
    if (dev_fd == -1)
        {
            fprintf(stderr, "failed to open device: %m\n");
            exit(1);
        }

    result = ioctl(dev_fd, ACLE_CMD_SET_RANDOM, args.use_software);
    if (result)
        {
            fprintf(stderr,
                    "failed call ioctl command: ret=%d(%s) errno=%m\n",
                    result, strerror(result));
            exit(1);
        }
    else
        {
            fprintf(stdout, "Switched to %s\n", args.use_software ? "SOFTWARE" : "HARDWARE");
        }

    close(dev_fd);
    return 0;
}
