#pragma once

//#include "asldef.hpp"
//#include "utils.hpp"
#include <stdint.h>

#define DISSALLOW_COPY_AND_ASSIGN(classname)  classname(const classname &);\
    classname & operator=(const classname &)

namespace jvchargeclient
{
class AESCrypt
{
    struct Context_t
    {
        uint32_t erk[64];	//块加密密匙
        uint32_t drk[64];	//块解密密匙
        int nr;				//块数
    };
public:
    AESCrypt() {}
    virtual ~AESCrypt() {}
public:
    //长度必须为128,192或256
    bool SetKey(uint8_t *pBuffer, uint32_t dwSize);
    bool SetKey(uint32_t dwIndex);
    bool SetKey();

    //加密，长度必须为16的整倍数
    void Encode(uint8_t *pDest, uint8_t *pSrc, uint32_t dwLength);

    //解密，长度必须为16的整倍数
    void Decode(uint8_t *pDest, uint8_t *pSrc, uint32_t dwLength);

private:
    static bool _SetKey(Context_t *ctx, uint8_t *key, int nbits);
    static void _Encode(Context_t *ctx, uint8_t input[16], uint8_t output[16]);
    static void _Decode(Context_t *ctx, uint8_t input[16], uint8_t output[16]);

protected:
    static void _GuessKey(uint8_t *pKey, uint32_t dwSize, uint32_t dwIndex);

private:
    Context_t m_Context;
    DISSALLOW_COPY_AND_ASSIGN(AESCrypt);
};
}

extern jvchargeclient::AESCrypt g_aes;
