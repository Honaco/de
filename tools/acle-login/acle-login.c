#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <stddef.h>

#include <fcntl.h>
#include <string.h>

#include "accord-le-ioctl.h"

#include "any-xid.h"

#define PRINT_XID

#define AMDZ_TMDEVICE_XID_LEN	8
#define AMDZ_ACCORDLE_XID_LEN	32
#define AMDZ_DB_PASSWORD_LEN	64

typedef struct {
    unsigned char tm_id[8];
    char          *password;
} Args;

void parse_args(int argc, char *argv[], Args *out_args)
{
    const char *help = "\
Usage:\n\
    acle-login TMID PASSWORD\n\
TMID must be passed as-is (serial not inverted/as returned from `tools/acle-get-tmid`)\n\
Options:\n\
    -h, --help - print this help\n";
    Args args;

    memset(&args, 0, sizeof(args));
    args.password = "";

    if(2 > argc)
        {
            fprintf(stderr, "%s", help);
            exit(1);
        }

    int got_login    = 0;
    int got_password = 0;

    int i = 1;
    for(i = 1; i < argc; i++)
        {
            if(!strcmp("-h", argv[i]) || !strcmp("--help", argv[i]))
                {
                    fprintf(stdout, "%s", help);
                }
            if(!got_login)
                {
                    uint8_t byte = 0;
                    unsigned j = 0;
                    for(j = 0; j < sizeof(args.tm_id); j++)
                        {
                            byte = 0;
                            unsigned jj = 0;
                            for(jj = 0; jj < 2; jj++)
                                {
                                    uint8_t half_byte = 0;
                                    if('\0' == argv[i][2*j+jj])
                                        {
                                            fprintf(stderr, "Too short argument '%s'\n", argv[i]);
                                            fprintf(stderr, "%s", help);
                                            exit(1);
                                        }
                                    else if('0' <= argv[i][2*j+jj]
                                             && argv[i][2*j+jj] <= '9')
                                        {
                                            half_byte = argv[i][2*j+jj] - '0';
                                        }
                                    else if('a' <= argv[i][2*j+jj]
                                             && argv[i][2*j+jj] <= 'f')
                                        {
                                            half_byte = argv[i][2*j+jj] - 'a' + 10;
                                        }
                                    else if('A' <= argv[i][2*j+jj]
                                             && argv[i][2*j+jj] <= 'F')
                                        {
                                            half_byte = argv[i][2*j+jj] - 'A' + 10;
                                        }
                                    byte = (byte << 4) + half_byte;
                                }
                            args.tm_id[j] = byte;
                        }
                    // В tmid 'нули идут в начале' - надо развернуть
                    // поле серийника - 6 байт, начиная со первого (в смысле
                    // с [1])
                    j = 0;
                    for(j = 0; j < 3; j++)
                        {
                            uint8_t swapped_val = args.tm_id[1 +     j];
                            args.tm_id[1 +     j] = args.tm_id[1 + 5 - j];
                            args.tm_id[1 + 5 - j] = swapped_val;
                        }
                    got_login = 1;
                }
            else if(!got_password)
                {
                    args.password = argv[i];
                    got_login = 1;
                }
        }
    if(!got_login)
        {
            fprintf(stderr, "%s", help);
            exit(1);
        }

    *out_args = args;
}

int main(int argc, char *argv[])
{
    int      dev_fd             = 0;
    int      result             = 0;
    uint8_t  xid[36]            = {};
    uint32_t userno             = 0;
    uint32_t user_flags         = 0;
    struct   acle_login_request req;
    Args     args;

    memset(&req,  0, sizeof(req));
    memset(&args, 0, sizeof(args));

    parse_args(argc, argv, &args);

    if (make_xid(args.password,
                 sizeof(args.tm_id),
                 args.tm_id,
                 sizeof(xid), xid))
        {
            fprintf(stderr, "failed to convert xid\n");
            exit(1);
        }
#ifdef PRINT_XID
    printf("xid: [");
    unsigned i = 0;
    for (i = 0; i < sizeof(xid); i++) {
        if (i != 0) {
            printf(" ");
        }
        printf("%02x", xid[i]);
    }
    printf("]\n");
#endif // PRINT_XID

    memcpy(req.hash, 4 + xid, sizeof(req.hash));
    req.userno     = &userno;
    req.user_flags = &user_flags;

    dev_fd = open("/dev/accord-le", O_RDWR);
    if (dev_fd == -1)
        {
            fprintf(stderr, "failed to open device: %m\n");
            exit(1);
        }

    result = ioctl(dev_fd, ACLE_CMD_LOGIN, &req);
    if (result)
        {
            fprintf(stderr,
                    "failed call ioctl command: ret=%d(%s) errno=%m\n",
                    result, strerror(result));
            exit(1);
        }
    else
        {
            fprintf(stdout, "\
Logged user no %u\n\
         flags ",
                userno);
            uint32_t bit = 1;
            unsigned i = 0;
            for (i = 0; i < 8*sizeof(user_flags); i++)
                {
                    fprintf(stdout, "%c", user_flags & bit ? '1' : '0');
                    if (0 == 1 + i % 16)
                        {
                            fprintf(stdout, "           ");
                        }
                    else if (0 == 1 + i % 8)
                        {
                            fprintf(stdout, "  ");
                        }
                    else if (0 == 1 + i % 4)
                        {
                            fprintf(stdout, " ");
                        }
                    bit <<= 1;
                }
            fprintf(stdout, "\n");
            fprintf(stdout, "Use acle-get-state to get more information about flags\n");
        }

    close(dev_fd);
    return 0;
}
