#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "accord-le-ioctl.h"

typedef struct {
    int verbose;
} Args;

typedef struct {
    unsigned   bit_no;
    const char *name;
} BIT_MAP;

BIT_MAP g_logged_user_flags[] = {
    // взято с libamdz/include/amdz.h
    {.bit_no = 0,
     .name   = "ADMIN"},
    {.bit_no = 1,
     .name   = "MAY_CHANGE_PASSWORD"},
    {.bit_no = 2,
     .name   = "BLOCKED"},
    // Altera-master/ACC_GX5/FPGA_GX/software/AccNios/src/inc/AMDZ.H
    {.bit_no = 3,
     .name   = "CAN_CREATE_USERS"},
};

BIT_MAP g_sw_flags[] = {
    {.bit_no = ACC_MISC_WD_ENABLED_BIT,  // 0
     .name   = "WATCHDOG_ENABLED"},
    {.bit_no = ACC_MISC_HAVE_TMI_BIT,    // 1
     .name   = "HAVE_TM_INPUT"},
    {.bit_no = ACC_MISC_HAVE_UID_BIT,    // 2
     .name   = "HAVE_VALID_UID"},
    {.bit_no = ACC_MISC_HARD_SN_BIT,     // 3
     .name   = "SERIAL_HARDCODED"},
    {.bit_no = ACC_MISC_HAVE_SDCARD_BIT, // 4
     .name   = "HAVE_SD_CARD"},
    {.bit_no = ACC_MISC_HAVE_RTC_BIT,    // 5
     .name   = "HAVE_RTC"},
    {.bit_no = ACC_MISC_INTRUDER_BIT,    // 6
     .name   = "INTRUDER_DETECTED"},
};

BIT_MAP g_hw_flags[] = {
    {.bit_no = log2(LE_SERVICE_MODE),                   // 0
     .name   = "SERVICE_MODE(deprecated)"},
    {.bit_no = log2(LE_ACCORD_MODE),                    // 1
     .name   = "ACCORD_MODE(deprecated)"},
    {.bit_no = log2(LE_WD_ON),                          // 2
     .name   = "WATCHDOG_ENABLED"},
    {.bit_no = log2(LE_HAVE_OWI),                       // 3
     .name   = "HAVE_ONE_WIRE_INTERFACE"}, // то же самое, что и ACC_MISC_TMI_BIT
    {.bit_no = log2(LE_HAVE_UID),                       // 4
     .name   = "HAVE_UID"},
    {.bit_no = log2(LE_HARD_SN),                        // 5
     .name   = "SERIAL_HARDCODED"},
    {.bit_no = log2(LE_HAVE_SDCARD),                    // 6
     .name   = "HAVE_SD_CARD"},
    {.bit_no = log2(LE_HAVE_RTC),                       // 7
     .name   = "HAVE_RTC"},
    {.bit_no = log2(LE_INTERNAL_FIRMWARE_CHECK_RESULT), // 28
     .name   = "INTERNAL_FIRMWARE_CHECK_RESULT"},
};

static const char* mode_to_str(uint8_t mode)
{
    switch(mode)
        {
            case LE_MODE_ON:
                return "ON";
            case LE_MODE_ADMIN:
                return "ADMIN";
            case LE_MODE_OFF:
                return "OFF";
            case LE_MODE_USER:
                return "USER";
            default:
                return "?UNDEFINED?";
        }
}

static const char* memtable_code_to_str(uint32_t code)
{
    switch(code)
        {
            case AREA_CODE_CONFIG:
                return "CONFIG";
            case AREA_CODE_BIOS:
                return "BIOS";
            case AREA_CODE_USER_BASE:
                return "USER_BASE";
            case AREA_CODE_COMMON:
                return "COMMON";
            case AREA_CODE_LOG:
                return "LOG";
            case AREA_CODE_OSIMG:
                return "OS_IMG";
            default:
                return "";
        }
}

static int find_bitmap_entry(unsigned bit_no,
                             BIT_MAP  *map,
                             unsigned bitmap_byte_size)
{
    unsigned bitmap_length = bitmap_byte_size / sizeof(*map);
    unsigned i = 0;
    for (i = 0; i < bitmap_length; i++)
        {
            if (bit_no == map[i].bit_no)
                {
                    return i;
                }
        }
    return -1;
}

static void print_bits(uint64_t bits,
                       unsigned sizeof_bit_field,
                       BIT_MAP  *map,
                       unsigned bitmap_byte_size,
                       Args     *args) {
    uint64_t cur        = 1;
    unsigned bits_count = 8*sizeof_bit_field;

    unsigned i = 0;
    for(i = 0; i < bits_count; i++, cur = cur<<1)
        {
            if(bits & cur)
                {
                    printf("1");
                }
            else
                {
                    printf("0");
                }
        }

    printf(" bin");

    if(bits)
        {
            unsigned use_comma = 0;
            cur = 1;
            printf(" {");
            unsigned i = 0;
            for(i = 0; i < bits_count; i++, cur = cur<<1)
                {
                    unsigned pos = find_bitmap_entry(i, map, bitmap_byte_size);
                    if((args->verbose && -1 != pos) || bits & cur)
                        {
                            if(use_comma)
                                {
                                    printf(",\n");
                                }
                            else
                                {
                                    use_comma = 1;
                                    printf("\n");
                                }
                            printf("        %3u bit = %u = ", i, bits & cur ? 1 : 0);
                            if(-1 == pos)
                                {
                                    printf("?UNDEFINED?");
                                }
                            else
                                {
                                    printf("%s", map[pos].name);
                                }
                        }
                }
            printf("\n    }");
        }
}

static void parse_args(int argc, char *argv[], Args *out_args)
{
    const char *help = "\
Print amdz state\n\
Usage:\n\
    acle-get-state [-v]\n\
Options:\n\
    -h, --help, help - show this message\n\
    -v, --verbose    - print verbose version";
    Args args;

    memset(&args, 0, sizeof(args));
    int i = 1;
    for(i = 1; i < argc; i++)
        {
            if(!strcmp("-h", argv[i])
               || !strcmp("--help", argv[i])
               || !strcmp("help", argv[i]))
                {
                    puts(help);
                    exit(0);
                }
            else if(!strcmp("-v", argv[i]) || !strcmp("--verbose", argv[i]))
                {
                    args.verbose = 1;
                }
        }
    *out_args = args;
}

static void print_memtable(memtable_t *memtable)
{
    printf("\
    memtable: {\n\
        signature: %02x%02x%02x%02x %02x%02x%02x%02x hex,\n\
        area_rec:  {\n",
        memtable->signature[0], memtable->signature[1],
        memtable->signature[2], memtable->signature[3],
        memtable->signature[4], memtable->signature[5],
        memtable->signature[6], memtable->signature[7]);
    unsigned i = 0;
    for(i = 0; i < NUM_AREAS; i++)
        {
            if(0 != i)
                {
                    printf(",\n");
                }
            printf("\
            {0x%08x-0x%08x, size=%10u(0x%08x), sector_pos=%3u(0x%02x), code=%-9s=%3u(0x%02x)}",
                memtable->area_rec[i].address,
                memtable->area_rec[i].address + memtable->area_rec[i].size
                    - (memtable->area_rec[i].size ? 1 : 0),
                memtable->area_rec[i].size,
                memtable->area_rec[i].size,
                memtable->area_rec[i].address / ACLE_SECTOR_SIZE,
                memtable->area_rec[i].address / ACLE_SECTOR_SIZE,
                memtable_code_to_str(memtable->area_rec[i].code),
                memtable->area_rec[i].code,
                memtable->area_rec[i].code);
        }
    printf("\n\
        }\n\
    },\n");
}

static void print_acle_state(struct acle_state *state, Args *args)
{
    puts("note: hw_flags and sw_flags are same at most + hw_flags may be not fully filled");
    uint8_t *bytes = NULL;
    puts("acle_state: {");
    printf("    mode:               %s (%u dec),\n", mode_to_str(state->mode), state->mode);
    bytes = (uint8_t*)&state->serial;
    printf("    serial:             %02x%02x%02x%02x hex,\n", bytes[0], bytes[1], bytes[2], bytes[3]);
    printf("    users_in_db:        %u dec,\n", state->nusers);
    printf("    logged_user_number: %u dec,\n", state->logged_userno);

    printf("    logged_user_flags:  ");
    print_bits(state->logged_user_flags,
               sizeof(state->logged_user_flags),
               g_logged_user_flags,
               sizeof(g_logged_user_flags),
               args);
    printf(",\n");

    printf("    hw_flags (in-hw-stored-on-pins): ");
    print_bits(state->hw_flags,
               sizeof(state->hw_flags),
               g_hw_flags,
               sizeof(g_hw_flags),
               args);
    printf(",\n");

    printf("    sw_flags (in-hw-computed):       ");
    print_bits(state->sw_flags,
               sizeof(state->sw_flags),
               g_sw_flags,
               sizeof(g_sw_flags),
               args);
    printf(",\n");

    printf("    fw_version:         %02x.%02x hex,\n", state->fw_ver_hi, state->fw_ver_lo);
    printf("    flash_id:           %08x hex,\n", state->flash_id);
    // printf(memtable);
    printf("    watchdog_timer:     %08x hex (%u dec),\n", state->watchdog_timer,state->watchdog_timer);
    print_memtable(&state->memtabel);
    printf("    board_uid:          %08x hex,\n", state->board_uid);
    printf("    flash_id_4:         %08x hex,\n", state->flash_id_4);
    printf("    sdc_first_sector:   %08x hex (%u dec)\n", state->sdc_first_sector,state->sdc_first_sector);
    printf("}\n");
}

int main(int argc, char *argv[])
{
    int res = 1, dev_fd = -1;
    struct acle_state state;
    Args args;

    memset(&state, 0, sizeof(state));
    memset(&args,  0, sizeof(args));

    parse_args(argc, argv, &args);

    dev_fd = open("/dev/accord-le", O_RDWR);
    if(dev_fd == -1)
        {
            fprintf(stderr, "Error: failed to open device: %m\n");
            goto out;
        }

    res = ioctl(dev_fd, ACLE_CMD_GET_STATE, &state);
    if(res)
        {
            fprintf(stderr, "ioctl failed: %m\n");
            goto out;
        }

    print_acle_state(&state, &args);

    res = 0;
out:
    close(dev_fd);
    return res;
}
