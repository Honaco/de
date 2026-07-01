#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

#include "accord-le-ioctl.h"

// Якобы определено в libamdz, по крайней мере так сказано в driver/accord-le.c
#define BUF0_SIZE 108

enum {
    READ = 1, // не убирать, исп. в switch-case
    WRITE,
    //TEST,
};

//static uint8_t g_buffer[BUF0_SIZE];

typedef struct
{
    // основное
    uint8_t    buffer[BUF0_SIZE];
    int        command;
    unsigned   bytes_count;
    const char *filename_to_use;
} Args;

static void parse_args(int argc, char *argv[], Args *out_args)
{
    const char *help = "\
Read/write buf0 to/from a file\n\
Usage:\n\
    acle-buf0 COMMAND FILE [BYTES_COUNT]\n\
\n\
Use BYTES_COUNT for WRITING ONLY to specify number of bytes to work with, up to default 108\n\
Commands:\n\
    help, -h, --help - show this message\n\
    read             - save read from buf0 data to file FILE\n\
    write            - write data to buf0 from file FILE\n";

    int  is_requested_exact_size = 0;
    Args args;

    memset(&args, 0, sizeof(args));

    //args.buffer = g_buffer;
    args.bytes_count = BUF0_SIZE;

    if (3 > argc)
        {
            fprintf(stdout, "%s", help);
            exit(1);
        }
    if (!strcmp("help", argv[1]) || !strcmp("--help", argv[1]) || !strcmp("-h", argv[1]))
        {
            fprintf(stdout, "%s", help);
            exit(0);
        }
    else
        {
            if (!strcmp("read", argv[1]))
                {
                    args.command = READ;
                }
            else if (!strcmp("write", argv[1]))
                {
                    args.command = WRITE;
                }
            else
                {
                    fprintf(stderr, "ERROR: unknown command '%s'\n", argv[1]);
                    fprintf(stderr, "%s", help);
                    exit(1);
                }

            args.filename_to_use = argv[2];

            if (3 < argc && WRITE == args.command)
                {
                    char *str_end = argv[3];
                    args.bytes_count = strtoul(argv[3], &str_end, 0);
                    if ('\0' != *str_end)
                        {
                            fprintf(stderr, "ERROR: failed to parse '%s' as number\n", argv[3]);
                            fprintf(stderr, "%s", help);
                            exit(1);
                        }
                    else if (BUF0_SIZE < args.bytes_count)
                        {
                            fprintf(stderr, "ERROR: BYTES_COUNT=%u is too large\n", args.bytes_count);
                            fprintf(stderr, "%s", help);
                            exit(1);
                        }
                    is_requested_exact_size = 1;
                }

            if (WRITE == args.command)
                {
                    FILE     *file     = NULL;
                    unsigned file_size = 0;

                    file = fopen(args.filename_to_use, "rb");
                    if (NULL == file)
                        {
                            fprintf(stderr, "ERROR: failed to open file '%s'\n", args.filename_to_use);
                            fprintf(stdout, "%s", help);
                            exit(1);
                        }
                    if (fseek(file, 0, SEEK_END))
                        {
                            fprintf(stderr, "ERROR: file operation 1 failed '%s'\n", args.filename_to_use);
                            exit(-1);
                        }
                    file_size = ftell(file);
                    if (fseek(file, 0, SEEK_SET))
                        {
                            fprintf(stderr, "ERROR: file operation 2 failed '%s'\n", args.filename_to_use);
                            exit(-1);
                        }
                    else if (args.bytes_count > file_size && is_requested_exact_size)
                        {
                            if (is_requested_exact_size)
                                {
                                    fprintf(stderr, "ERROR: file must be %u bytes size\n", BUF0_SIZE);
                                    exit(-1);
                                }
                            else
                                {
                                    fprintf(stderr, "Warning: contains only %u bytes, which is less than default %u\n", file_size, BUF0_SIZE);
                                    args.bytes_count = file_size;
                                }
                        }

                    file_size = fread(args.buffer, 1, args.bytes_count, file);
                    if (args.bytes_count != file_size)
                        {
                            fprintf(stderr, "ERROR: could read only %u (0x%x) of %u (0x%x) bytes from file '%s'\n", file_size, file_size, BUF0_SIZE, BUF0_SIZE, args.filename_to_use);
                            exit(-1);
                        }
                    fclose(file);
                }
        }

    *out_args = args;
}

int main(int argc, char *argv[])
{
    int res = -1, dev_fd = -1, acle_command = 0;
    unsigned bytes_count = 0;
    void     *ptr_to_pass = NULL;
    struct acle_data_chunk req;
    Args args;

    memset(&args, 0, sizeof(args));
    memset(&req, 0, sizeof(req));

    parse_args(argc, argv, &args);

    dev_fd = open("/dev/accord-le", O_RDWR);
    if (dev_fd == -1) {
        fprintf(stderr, "failed to open device: %m\n");
        exit(-1);
    }

    switch (args.command)
        {
            //case TEST:
            //    break;
            case READ:
                acle_command = ACLE_CMD_READ_FROM_BUF0;
                ptr_to_pass  = args.buffer;
            case WRITE:
                if (!acle_command)
                    {
                        acle_command = ACLE_CMD_WRITE_TO_BUF0;
                        req.count = args.bytes_count;
                        req.data  = args.buffer;
                        ptr_to_pass = &req;
                    }

                res = ioctl(dev_fd, acle_command, ptr_to_pass);
                if (res)
                    {
                        fprintf(stderr, "ioctl failed: %m\n");
                        goto out;
                    }
                if (READ == args.command)
                    {
                        FILE *file = fopen(args.filename_to_use, "wb");
                        bytes_count = fwrite(args.buffer, 1, args.bytes_count, file);
                        if (args.bytes_count != bytes_count)
                            {
                                fprintf(stderr, "ERROR: could write only %u (0x%x) of %u (0x%x) bytes from file '%s'\n", bytes_count, bytes_count, args.bytes_count, args.bytes_count, args.filename_to_use);
                                res = -1;
                            }
                        fclose(file);
                    }
                fprintf(stdout, "Done!\n");
                break;
            default:
                fprintf(stderr, "internal error %s:%u\n", __FILE__, __LINE__);
                goto out;
        }

    res = 0;
out:
    close(dev_fd);
    return res;
}
