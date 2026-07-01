#include <string.h>

#include "any-xid.h"

#define AMDZ_TMDEVICE_XID_LEN	8
#define AMDZ_ACCORDLE_XID_LEN	32
#define AMDZ_DB_PASSWORD_LEN	64

#pragma pack(push, 1)

/** Свертка(хеш) пароля */
typedef struct
{
    /** Класс устройств */
    int dev_class;
    /** Свертка */
    union
    {
        /** Свертка для устройств, обслуживаемых драйвером tmdevice */
        uint8_t tmdevice[AMDZ_TMDEVICE_XID_LEN];
        /** Свертка для устройств, обслуживаемых драйвером accord-le */
        uint8_t accordle[AMDZ_ACCORDLE_XID_LEN];
    } xid;
} amdz_xid_t;

/**
 * @ingroup tmid
 * Описание ТМ-идентификатора
 */
struct amdz_tmid
{
    /** Тип */
    uint8_t type;
    /** Серийный номер */
    uint8_t sn[6];
    /** CRC от типа и серийного номера */
    uint8_t crc;
};

#pragma pack(pop)


static uint32_t g_seed;


#ifndef OS_QNX
static uint8_t get_random_byte(void)
{
    uint64_t edx = 0x0000000008088405LLU;

    edx   *= g_seed;
    g_seed = (uint32_t)edx + 1;
    edx    = (uint64_t)g_seed * 256;

    return (uint8_t)(edx >> 32);
}
#else
/* Watcom 10.6, that is used in QNX 4.25, doesn't know anything about
 * 64-bit types, so we need to code get_random_byte() in good old assembler
 */
extern uint8_t get_random_byte(void);
#pragma aux get_random_byte = \
    "mov eax, g_seed" \
    "mov edx, 8088405h" \
    "mul edx" \
    "inc eax" \
    "mov g_seed, eax" \
    "mov edx, 256" \
    "mul edx" \
    "mov eax, edx" \
    value [al] \
    modify [eax edx];
/*   "mov res, al" \ */
#endif /* OS_QNX */

static void xor_random_data(char *data, size_t size)
{
    uint32_t i;

    for (i = 0; i < size; i++)
        data[i] ^= get_random_byte();
}

static void _make_xid(const struct amdz_tmid *tmid, uint8_t *pass, char *xid)
{
    char TTP[AMDZ_DB_PASSWORD_LEN + 1 + sizeof(struct amdz_tmid)];
    uint32_t RS;
    char *Adr;
    int i;

    memcpy(xid, tmid, sizeof(struct amdz_tmid));
    memcpy(TTP, pass, AMDZ_DB_PASSWORD_LEN + 1);
    for (i = TTP[0] + 1; i <= AMDZ_DB_PASSWORD_LEN; i++)
        TTP[i] = (char)i;

    g_seed = 0x10051964;
    xor_random_data(&TTP[1], AMDZ_DB_PASSWORD_LEN);

    for (i = 0; i < 3; i++) {
        Adr = &TTP[i * 4] + 1;
        memcpy(&RS, Adr, 4);
        g_seed = (uint32_t)RS;
        xor_random_data(xid, 8);
    }
}

//int make_xid(struct amdz_tmid *tmid, char *password, amdz_xid_t *xid)
int make_xid(char     *in_ascii_password,
             unsigned in_tmid_size,
             void     *in_tmid,
             unsigned in_xid_size,
             void     *out_xid)
{
    //int dev_type = 1; // AMDZ_DEV_CLASS_ACCORDLE
    uint8_t acpass[AMDZ_DB_PASSWORD_LEN + 1] = {0};
    int i = 0;

    size_t           password_len = in_ascii_password
                                    ? strlen(in_ascii_password) : 0;
    struct amdz_tmid *tmid        = (struct amdz_tmid*)in_tmid;
    amdz_xid_t       xid;

    memset(&xid, 0xff, sizeof(xid));

    xid.dev_class = 2; // AMDZ_DEV_CLASS_ACCORDLE

    if (AMDZ_DB_PASSWORD_LEN < password_len)
        return -1; // ACIA_ecInvalidParameters;
    else if (sizeof(*tmid) != in_tmid_size || sizeof(xid) != in_xid_size)
        return -1; // ACIA_ecInvalidParameters;

    if (password_len) {
        strncpy((char *)&acpass + 1, in_ascii_password, AMDZ_DB_PASSWORD_LEN);
        *(char *)acpass = password_len;
    }

    _make_xid(tmid, acpass, 4 + (char*)&xid);

    /* erase password data */
    for (i = 0; i < (int)sizeof(acpass); ++i)
        *((uint8_t *)acpass + i) = get_random_byte();

    *(amdz_xid_t*)out_xid = xid;
    return 0; // ACIA_ecOK
}
