#include <stdint.h>
#include <stddef.h>

int make_xid(char     *in_ascii_password,
             unsigned in_tmid_size,
             void     *in_tmid,
             unsigned in_xid_side,
             void     *out_xid);
