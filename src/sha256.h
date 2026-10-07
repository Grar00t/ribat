#ifndef RIBAT_SHA256_H
#define RIBAT_SHA256_H

typedef unsigned char ribat_u8;
typedef unsigned int ribat_u32;
typedef unsigned long ribat_u64;

typedef struct {
    ribat_u32 h[8];
    ribat_u64 total;
    ribat_u32 used;
    ribat_u8 block[64];
} RibatSha256;

void ribat_sha256_init(RibatSha256 *ctx);
int ribat_sha256_update(RibatSha256 *ctx, const void *data, ribat_u64 len);
void ribat_sha256_final(RibatSha256 *ctx, ribat_u8 out[32]);
int ribat_sha256(const void *data, ribat_u64 len, ribat_u8 out[32]);

#endif
