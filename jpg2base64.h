#pragma once
#include <string>

class Jpg2Base64
{
public:
    Jpg2Base64();
    ~Jpg2Base64();
public:
    //jpg file read from disk.
    void Convert(char *jpgname, char* out_base64, int &out_len);
private:
    int base64_encode(const unsigned char *in,  unsigned long len, unsigned char *out);
    int base64_decode(const unsigned char *in, unsigned char *out);
};
