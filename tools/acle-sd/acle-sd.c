#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

#include "accord-le-ioctl.h"

enum {
    READ = 1, // не убирать, исп. в switch-case
    WRITE,
    //TEST,
};

typedef struct
{
    // основное
    uint8_t    *buffer;
    unsigned   buffer_size;
    unsigned   sector_number;
    unsigned   offset_in_sector;
    int        command;
    // дополнительное
    int        is_buffer_allocated;
    int        use_raw;
    const char *filename_to_use;
} Args;

struct
{
    const char *command_name;
    int        minimum_argc;
    int        use_file;
    int        use_raw;
    int        command;
} g_available_args[] = {
    {
        .command_name = "read-file",
        .minimum_argc = 5,
        .use_file     = 1,
        .use_raw      = 0,
        .command      = READ,
    },
    {
        .command_name = "write-file",
        .minimum_argc = 4,
        .use_file     = 1,
        .use_raw      = 0,
        .command      = WRITE,
    },
    {
        .command_name = "read-bytes",
        .minimum_argc = 4,
        .use_file     = 0,
        .use_raw      = 0,
        .command      = READ,
    },
    {
        .command_name = "write-bytes",
        .minimum_argc = 4,
        .use_file     = 0,
        .use_raw      = 0,
        .command      = WRITE,
    },
    {
        .command_name = "read-str",
        .minimum_argc = 4,
        .use_file     = 0,
        .use_raw      = 1,
        .command      = READ,
    },
    {
        .command_name = "write-str",
        .minimum_argc = 4,
        .use_file     = 0,
        .use_raw      = 1,
        .command      = WRITE,
    },
};

int is_command_valid (const char *in_command_str,
                      int        *out_minimum_argc,
                      int        *out_use_file,
                      int        *out_use_raw,
                      int        *out_command)
{
    unsigned i = 0;
    for (i = 0;
         i < sizeof(g_available_args)/sizeof(*g_available_args);
         i++)
        {
            if (!strcmp(in_command_str, g_available_args[i].command_name))
                {
                    *out_minimum_argc = g_available_args[i].minimum_argc;
                    *out_use_file     = g_available_args[i].use_file;
                    *out_use_raw      = g_available_args[i].use_raw;
                    *out_command      = g_available_args[i].command;
                    return 1;
                }
        }
    return 0;
}

void usage (char *argv_0)
{
    const char *usage_str = "\
Read/write/test sd-card\n\
Usage:\n\
    %s COMMAND [BYTE-OFFSET [BYTE-SIZE]] [ARGUMENT]\n\
where BYTE-OFFSET and BYTE-SIZE are decimal or '0x'-prefixed hexanical,\n\
BYTE-OFFSET is required everywhere except 'help' and 'test' commands,\n\
BYTE-SIZE is required when reading\n\
\n\
Note: BYTE-SIZE larger than 512 is unsupported by a hardware\n\
\n\
Commands:\n\
    help, -h, --help - show this message\n\
    read-file        - save read from sd data to file, set by ARGUMENT\n\
    write-file       - write data to sd from file, set byte ARGUMENT\n\
    read-bytes       - print read from sd data as hexanical bytes\n\
    write-bytes      - parse ARGUMENT as bytes (string [0-9a-fA-F])\n\
    read-str         - print read from sd data as-is\n\
    write-str        - write data to as from ARGUMENT as string (as-is)\n\
EXAMPLES:\n\
    Read 10 bytes at offset 20 into file `out.bin`\n\
      %s read-file 20 10 out.bin\n\
\n\
    Write at offset 22 4 bytes, passed at str\n\
      %s write-bytes 22 ab016F42\n\
\n\
    Print 4 bytes at offset 8 as ascii-string\n\
      %s read-str 8 4\n";
    //test             - test sd automatically, ignores BYTE-OFFSET and BYTE-SIZE\n

    fprintf(stderr, usage_str, argv_0, argv_0, argv_0, argv_0);
}

void parse_args(int argc, char *argv[], Args *out_args)
{

    int  minimum_argc     = 0;
    int  use_file         = 0;
    Args args;

    memset(&args, 0, sizeof(args));

    if (2 > argc)
        {
            usage(argv[0]);
            exit(0);
        }
    if (!strcmp("help", argv[1]) || !strcmp("--help", argv[1]) || !strcmp("-h", argv[1]))
        {
            usage(argv[0]);
            exit(0);
        }
    //else if (!strcmp("test", argv[1]))
    //    {
    //        args.command = TEST;
    //    }
    else if (is_command_valid(argv[1], &minimum_argc, &use_file, &args.use_raw, &args.command))
        {
            // read or write
            if (minimum_argc > argc)
                {
                    fprintf(stderr, "ERROR: the command '%s' requires %u arguments\n", argv[1], minimum_argc);
                    usage(argv[0]);
                    exit(1);
                }
            char     *end_str = NULL;
            unsigned number   = 0;

            // auto-detects 0x, 0X and 0 prefixes
            number = strtoul(argv[2], &end_str, 0);
            if ('\0' != *end_str)
                {
                    fprintf(stderr, "ERROR: invalid characted found at argument '%s'\n", argv[2]);
                    usage(argv[0]);
                    exit(1);
                }
            args.sector_number    = number / ACLE_MAX_SD_BLOCK_SIZE;
            args.offset_in_sector = number % ACLE_MAX_SD_BLOCK_SIZE;

            // set buffer_size
            if (READ == args.command)
                {
                    // read
                    args.buffer_size = strtoul(argv[3], &end_str, 0);
                    if ('\0' != *end_str)
                        {
                            fprintf(stderr, "ERROR: invalid characted found at argument '%s'\n", argv[2]);
                            usage(argv[0]);
                            exit(1);
                        }

                    args.is_buffer_allocated = 1;
                    if (NULL == (args.buffer = calloc(1, args.buffer_size)))
                        {
                            fprintf(stderr, "ERROR: failed to allocate %u (0x%x) bytes\n", args.buffer_size, args.buffer_size);
                            exit(-1);
                        }

                    if (use_file)
                        {
                            args.filename_to_use = argv[4];
                        }
                }
            else
                {
                    // write
                    if (args.use_raw)
                        {
                            args.is_buffer_allocated = 0;
                            args.buffer = (uint8_t*)argv[3];

                            args.buffer_size = strlen(argv[3]);
                        }
                    else
                        {
                            args.is_buffer_allocated = 1;
                            if (use_file)
                                {
                                    FILE     *file            = NULL;
                                    unsigned bytes_read_count = 0;

                                    args.filename_to_use = argv[3];

                                    file = fopen(args.filename_to_use, "rb");
                                    if (NULL == file)
                                        {
                                            fprintf(stderr, "ERROR: failed to open file '%s'\n", args.filename_to_use);
                                            usage(argv[0]);
                                            exit(1);
                                        }
                                    if (fseek(file, 0, SEEK_END))
                                        {
                                            fprintf(stderr, "ERROR: file operation 1 failed '%s'\n", args.filename_to_use);
                                            exit(-1);
                                        }
                                    args.buffer_size = ftell(file);
                                    if (fseek(file, 0, SEEK_SET))
                                        {
                                            fprintf(stderr, "ERROR: file operation 2 failed '%s'\n", args.filename_to_use);
                                            exit(-1);
                                        }

                                    if (NULL == (args.buffer = calloc(1, args.buffer_size)))
                                        {
                                            fprintf(stderr, "ERROR: failed to allocate %u (0x%x) bytes\n", args.buffer_size, args.buffer_size);
                                            exit(-1);
                                        }

                                    bytes_read_count = fread(args.buffer, 1, args.buffer_size, file);
                                    if (args.buffer_size != bytes_read_count)
                                        {
                                            fprintf(stderr, "ERROR: could read only %u (0x%x) of %u (0x%x) bytes from file '%s'\n", bytes_read_count, bytes_read_count, args.buffer_size, args.buffer_size, args.filename_to_use);
                                            exit(-1);
                                        }
                                    fclose(file);
                                }
                            else
                                {
                                    uint8_t  byte = 0;
                                    char     *str = argv[3];
                                    unsigned len  = strlen(str);
                                    if (0 != len % 2)
                                        {
                                            fprintf(stderr,
                                                    "ERROR: failed to parse as bytes '%s' - length must be a multiple of 2\n", argv[4]);
                                            free(args.buffer);
                                            exit(1);
                                        }
                                    args.buffer_size = len / 2;

                                    if (NULL == (args.buffer = calloc(1, args.buffer_size)))
                                        {
                                            fprintf(stderr, "ERROR: failed to allocate %u (0x%x) bytes\n", args.buffer_size, args.buffer_size);
                                            exit(-1);
                                        }
                                    unsigned i = 0;
                                    for (i = 0; i < args.buffer_size; i++)
                                        {
                                            byte = 0;
                                            unsigned j = 0;
                                            for (j = 0; j < 2; j++)
                                                {
                                                    if ('0' <= str[2*i+j] && str[2*i+j] <= '9')
                                                        {
                                                            byte = (byte << 4) + str[2*i+j] - '0';
                                                        }
                                                    else if ('a' <= str[2*i+j] && str[2*i+j] <= 'f')
                                                        {
                                                            byte = (byte << 4) + str[2*i+j] - 'a' + 10;
                                                        }
                                                    else if ('A' <= str[2*i+j] && str[2*i+j] <= 'F')
                                                        {
                                                            byte = (byte << 4) + str[2*i+j] - 'A' + 10;
                                                        }
                                                    else
                                                        {
                                                            fprintf(stderr,
                                                                    "ERROR: failed to parse as bytes '%s' - unexpected symbol '%c'\n", argv[4], str[2*i+j]);
                                                            free(args.buffer);
                                                            exit(1);
                                                        }
                                                }
                                            args.buffer[i] = byte;
                                        }
                                }
                        }
                }
        }
    else
        {
            fprintf(stderr, "ERROR: failed to recognize command '%s'\n", argv[1]);
            usage(argv[0]);
            exit(1);
        }

    *out_args = args;
}

void run_test (int dev_fd)
{
}

int main(int argc, char *argv[])
{
    int res = -1, dev_fd = -1, acle_command = 0;
    unsigned bytes_count = 0;
    struct acle_sd_chunk req;
    Args args;

    memset(&args, 0, sizeof(args));
    memset(&req, 0, sizeof(req));

    parse_args(argc, argv, &args);

    dev_fd = open("/dev/accord-le", O_RDWR);
    if (dev_fd == -1) {
        fprintf(stderr, "failed to open device: %m\n");
        goto out;
    }

    switch (args.command)
        {
            //case TEST:
            //    break;
            case READ:
                acle_command = ACLE_CMD_SD_READ;
            case WRITE:
                if (!acle_command)
                    {
                        acle_command = ACLE_CMD_SD_WRITE;
                    }
                req.sector_number = args.sector_number;
                req.offset        = args.offset_in_sector;
                req.size          = args.buffer_size;
                req.data          = args.buffer;

                res = ioctl(dev_fd, acle_command, &req);
                if (res)
                    {
                        fprintf(stderr, "ioctl failed: %m\n");
                        goto out;
                    }
                if (READ == args.command)
                    {
                        if (args.use_raw)
                            {
                                bytes_count = fwrite(args.buffer, 1, args.buffer_size, stdout);
                                if (args.buffer_size != bytes_count)
                                    {
                                        fprintf(stderr, "ERROR: could print only %u (0x%x) of %u (0x%x) bytes\n", bytes_count, bytes_count, args.buffer_size, args.buffer_size);
                                        res = -1;
                                    }
                                fprintf(stdout, "\n");
                            }
                        else if (args.filename_to_use)
                            {
                                FILE *file = fopen(args.filename_to_use, "wb");
                                bytes_count = fwrite(args.buffer, 1, args.buffer_size, file);
                                if (args.buffer_size != bytes_count)
                                    {
                                        fprintf(stderr, "ERROR: could write only %u (0x%x) of %u (0x%x) bytes from file '%s'\n", bytes_count, bytes_count, args.buffer_size, args.buffer_size, args.filename_to_use);
                                        res = -1;
                                    }
                                fclose(file);
                            }
                        else
                            {
                                unsigned i = 0;
                                        for (i = 0; i < args.buffer_size; i++)
                                            {
                                                fprintf(stdout, " ");
                                                if (0 == i % 16)
                                                {
                                                    fprintf(stdout, "\n");
                                                }
                                                if (0 == i % 4)
                                                    {
                                                        fprintf(stdout, " ");
                                                    }
                                                fprintf(stdout, " %02x", args.buffer[i]);
                                            }
                                fprintf(stdout, "\n");
                            }
                    }
                fprintf(stdout, "Done!\n");
                break;
            default:
                fprintf(stderr, "internal error %s:%u\n", __FILE__, __LINE__);
                goto out;
        }
    res = 0;

out:
    if (args.is_buffer_allocated)
        {
            free(args.buffer);
        }

    close(dev_fd);
    return res;
}
