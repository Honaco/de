#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "accord-le-ioctl.h"

typedef struct {
    unsigned long bytes_count;
    char          *dump_desired_filename;
    int           enable_dump;
} Args;

static void parse_args(int argc, char *argv[], Args *out_args)
{
    const char *help = "\
Get random bytes from amdz. Print or dump.\n\
Usage:\n\
    acle-get-random NUM_BYTES [-o [desired_filename]]\n\
Options:\n\
    -h, --help, help      - show this help\n\
    -o [desired_filename] - dump random bytes,\n\
                            use `desired_filename` as output file name if\n\
                            mentioned (default filename is 'rand-yyyy-mm-dd-hh-mm-ss')";
    Args args;
    memset(&args, 0, sizeof(args));

    int is_bytes_count_unset = 1;

    if(2 > argc)
        {
            fprintf(stderr, "Error: at least 1 argument required");
            puts(help);
            exit(-1);
        }
    int i = 0;
    for(i = 1; i < argc; i++)
        {
            if(!strcmp("-h", argv[i])
               || !strcmp("--help", argv[i])
               || !strcmp("help", argv[i]))
                {
                    puts(help);
                    exit(0);
                }
            else if(!strcmp("-o", argv[i]))
                {
                    args.enable_dump = 1;
                }
            else if(!strcmp("-o", argv[i - 1]))
                {
                    args.dump_desired_filename = argv[i];
                }
            else if(is_bytes_count_unset)
                {
                    is_bytes_count_unset = 0;
                    char *str_end = NULL;
                    // Автоматически парсит 0x*, 0X* и 0*
                    args.bytes_count = strtoul(argv[i], &str_end, 0);
                    if ('\0' != *str_end)
                        {
                            fprintf(stderr, "Error: can't parse '%s' as number\n", argv[i]);
                            exit(-1);
                        }
                }
        }
    if(is_bytes_count_unset)
        {
            fprintf(stderr, "Error: bytes count to retrieve was not passed\n");
            if(args.dump_desired_filename)
                {
                    fprintf(stderr,
                            "Note: '%s' was parsed as dump desired filename\n",
                            args.dump_desired_filename);
                }
            exit(-1);
        }
    *out_args = args;
}

int main(int argc, char *argv[])
{
    int i = 0, res = 1, dev_fd = -1;
    struct acle_rng_request req;
    Args args;

    memset(&req, 0, sizeof(req));
    memset(&args, 0, sizeof(args));

    parse_args(argc, argv, &args);

    req.buf = calloc(args.bytes_count, 1);
    if(!req.buf)
        {
            fprintf(stderr, "Error: failed to allocate %lu bytes for buffer\n", args.bytes_count);
            exit(EXIT_FAILURE);
        }
    req.count = args.bytes_count;

    dev_fd = open("/dev/accord-le", O_RDWR);
    if(dev_fd == -1)
        {
            fprintf(stderr, "Error: failed to open device: %m\n");
            goto out1;
        }

    res = ioctl(dev_fd, ACLE_CMD_GET_RANDOM_BYTES, &req);
    if(res)
        {
            fprintf(stderr, "ioctl failed: %m\n");
            goto out2;
        }

    printf("got %lu random byte(s):\n", req.count);
    if(args.enable_dump)
        {
            char   now_str[32] = "rand-yyyy-mm-dd-hh-mm-ss";
            time_t now         = time(NULL);
            FILE   *file       = NULL;
            strftime(now_str + 5, sizeof(now_str) - 5, "%Y-%m-%d-%H-%M-%S", localtime(&now));
            if(!args.dump_desired_filename)
                {
                    args.dump_desired_filename = now_str;
                }
            file = fopen(args.dump_desired_filename, "w");
            if(!file)
                {
                    printf("Warning: failed to open file '%s' for randoms dump\n",
                           args.dump_desired_filename);
                }
            else
                {
                    fwrite(req.buf, 1, req.count, file);
                    fclose(file);
                    printf("\n Dump saved as '%s'\n", args.dump_desired_filename);
                }
        }
    else
        {
            for (i = 0; i < req.count; ++i)
                printf("%2.2x ", *((uint8_t *)req.buf + i));
        }
    printf("\n");

    res = 0;
out2:
    close(dev_fd);
out1:
    free(req.buf);
    return res;
}
