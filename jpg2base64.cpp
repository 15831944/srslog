#include "jpg2base64.h"
#include <stdio.h>

Jpg2Base64::Jpg2Base64()
{

}

Jpg2Base64::~Jpg2Base64()
{

}

void Jpg2Base64::Convert(char *jpgname, char*out_base64, int &out_len)
{
    FILE *fjpg = fopen(jpgname, "rb");
    if (NULL == fjpg)
    {
        return;
    }

    enum {JPGBUFF_SIZE = 1024 *1024};
    char *jpgbuff = new char [JPGBUFF_SIZE];
    if (NULL == jpgbuff)
    {
        return;
    }

    int actuall_read = fread(jpgbuff, 1, JPGBUFF_SIZE, fjpg);
    out_len = base64_encode((unsigned char *)jpgbuff, actuall_read, (unsigned char *)out_base64);

    if (NULL != fjpg)
    {
        fclose(fjpg);
    }
    if (NULL != jpgbuff)
    {
        delete [] jpgbuff;
        jpgbuff = NULL;
    }
}

static const char *codes =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const unsigned char map[256] = {
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 253, 255,
255, 253, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 253, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255,  62, 255, 255, 255,  63,
 52,  53,  54,  55,  56,  57,  58,  59,  60,  61, 255, 255,
255, 254, 255, 255, 255,   0,   1,   2,   3,   4,   5,   6,
  7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,
 19,  20,  21,  22,  23,  24,  25, 255, 255, 255, 255, 255,
255,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,
 37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
 49,  50,  51, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255 };

int Jpg2Base64::base64_encode(const unsigned char *in,  unsigned long len,
                        unsigned char *out)
{
   unsigned long i, len2, leven;
   unsigned char *p;
   /* valid output size ? */
   len2 = 4 * ((len + 2) / 3);
   p = out;
   leven = 3*(len / 3);
   for (i = 0; i < leven; i += 3) {
       *p++ = codes[in[0] >> 2];
       *p++ = codes[((in[0] & 3) << 4) + (in[1] >> 4)];
       *p++ = codes[((in[1] & 0xf) << 2) + (in[2] >> 6)];
       *p++ = codes[in[2] & 0x3f];
       in += 3;
   }
   /* Pad it if necessary...  */
   if (i < len) {
       unsigned a = in[0];
       unsigned b = (i+1 < len) ? in[1] : 0;
       unsigned c = 0;

       *p++ = codes[a >> 2];
       *p++ = codes[((a & 3) << 4) + (b >> 4)];
       *p++ = (i+1 < len) ? codes[((b & 0xf) << 2) + (c >> 6)] : '=';
       *p++ = '=';
   }

   /* append a NULL byte */
   *p = '\0';

   return p - out;
}

int Jpg2Base64::base64_decode(const unsigned char *in, unsigned char *out)
{
    unsigned long t, x, y, z;
    unsigned char c;
    int	g = 3;

    for (x = y = z = t = 0; in[x]!=0;) {
        c = map[in[x++]];
        if (c == 255) return -1;
        if (c == 253) continue;
        if (c == 254) { c = 0; g--; }
        t = (t<<6)|c;
        if (++y == 4) {
//			if (z + g > *outlen) { return CRYPT_BUFFER_OVERFLOW; }
            out[z++] = (unsigned char)((t>>16)&255);
            if (g > 1) out[z++] = (unsigned char)((t>>8)&255);
            if (g > 2) out[z++] = (unsigned char)(t&255);
            y = t = 0;
        }
    }
//	if (y != 0) {
//		return -1;
//	}
    return z;
}


