/* Stolen from : https://github.com/lcn2/fnv */

#include "hash.h"

Fnv32_t fnv_32a_str(const char *str, int length)
{
    Fnv32_t hval = 0;
    unsigned char *s = (unsigned char *)str; /* unsigned string */

    /*
     * FNV-1a hash each octet in the buffer
     */
    for (int i = 0; i < length; ++i)
    {

        /* xor the bottom with the current octet */
        hval ^= (Fnv32_t)*s++;

        /* multiply by the 32 bit FNV magic prime mod 2^32 */
#if defined(NO_FNV_GCC_OPTIMIZATION)
        hval *= FNV_32_PRIME;
#else
        hval += (hval << 1) + (hval << 4) + (hval << 7) + (hval << 8) + (hval << 24);
#endif
    }

    /* return our new hash value */
    return hval;
}
