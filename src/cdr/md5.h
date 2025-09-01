#ifndef MD5_H
#define MD5_H

#include "travodds_dll.h"
#include <stdint.h>

TRAVODDS_NAMESPACE_BEGIN

class MD5 {
public:
    MD5();
    void update(const uint8_t* input, uint32_t inputlen);
    void finalize(uint8_t digest[16]);

private:
    void transform(const uint8_t block[64]);
    void encode(uint8_t* output, const uint32_t* input, uint32_t len);
    void decode(uint32_t* output, const uint8_t* input, uint32_t len);

    uint32_t count[2];
    uint32_t state[4];
    uint8_t buffer[64];

    static const uint8_t PADDING[64];
};

TRAVODDS_NAMESPACE_END

#endif

