#include "sha256.h"

static const ribat_u32 K[64] = {
    0x428a2f98U,0x71374491U,0xb5c0fbcfU,0xe9b5dba5U,0x3956c25bU,0x59f111f1U,0x923f82a4U,0xab1c5ed5U,
    0xd807aa98U,0x12835b01U,0x243185beU,0x550c7dc3U,0x72be5d74U,0x80deb1feU,0x9bdc06a7U,0xc19bf174U,
    0xe49b69c1U,0xefbe4786U,0x0fc19dc6U,0x240ca1ccU,0x2de92c6fU,0x4a7484aaU,0x5cb0a9dcU,0x76f988daU,
    0x983e5152U,0xa831c66dU,0xb00327c8U,0xbf597fc7U,0xc6e00bf3U,0xd5a79147U,0x06ca6351U,0x14292967U,
    0x27b70a85U,0x2e1b2138U,0x4d2c6dfcU,0x53380d13U,0x650a7354U,0x766a0abbU,0x81c2c92eU,0x92722c85U,
    0xa2bfe8a1U,0xa81a664bU,0xc24b8b70U,0xc76c51a3U,0xd192e819U,0xd6990624U,0xf40e3585U,0x106aa070U,
    0x19a4c116U,0x1e376c08U,0x2748774cU,0x34b0bcb5U,0x391c0cb3U,0x4ed8aa4aU,0x5b9cca4fU,0x682e6ff3U,
    0x748f82eeU,0x78a5636fU,0x84c87814U,0x8cc70208U,0x90befffaU,0xa4506cebU,0xbef9a3f7U,0xc67178f2U
};

static ribat_u32 rotr(ribat_u32 x, unsigned n) { return (x >> n) | (x << (32U - n)); }
static ribat_u32 load_be32(const ribat_u8 *p) {
    return ((ribat_u32)p[0] << 24) | ((ribat_u32)p[1] << 16) | ((ribat_u32)p[2] << 8) | p[3];
}
static void store_be32(ribat_u8 *p, ribat_u32 v) {
    p[0]=(ribat_u8)(v>>24); p[1]=(ribat_u8)(v>>16); p[2]=(ribat_u8)(v>>8); p[3]=(ribat_u8)v;
}
static void compress(RibatSha256 *c, const ribat_u8 b[64]) {
    ribat_u32 w[64], a,bv,cc,d,e,f,g,h,t1,t2;
    unsigned i;
    for (i=0;i<16;i++) w[i]=load_be32(b+4*i);
    for (i=16;i<64;i++) {
        ribat_u32 s0=rotr(w[i-15],7)^rotr(w[i-15],18)^(w[i-15]>>3);
        ribat_u32 s1=rotr(w[i-2],17)^rotr(w[i-2],19)^(w[i-2]>>10);
        w[i]=w[i-16]+s0+w[i-7]+s1;
    }
    a=c->h[0]; bv=c->h[1]; cc=c->h[2]; d=c->h[3]; e=c->h[4]; f=c->h[5]; g=c->h[6]; h=c->h[7];
    for (i=0;i<64;i++) {
        ribat_u32 s1=rotr(e,6)^rotr(e,11)^rotr(e,25);
        ribat_u32 ch=(e&f)^((~e)&g);
        ribat_u32 s0=rotr(a,2)^rotr(a,13)^rotr(a,22);
        ribat_u32 maj=(a&bv)^(a&cc)^(bv&cc);
        t1=h+s1+ch+K[i]+w[i]; t2=s0+maj;
        h=g; g=f; f=e; e=d+t1; d=cc; cc=bv; bv=a; a=t1+t2;
    }
    c->h[0]+=a; c->h[1]+=bv; c->h[2]+=cc; c->h[3]+=d; c->h[4]+=e; c->h[5]+=f; c->h[6]+=g; c->h[7]+=h;
}
void ribat_sha256_init(RibatSha256 *c) {
    static const ribat_u32 iv[8]={0x6a09e667U,0xbb67ae85U,0x3c6ef372U,0xa54ff53aU,0x510e527fU,0x9b05688cU,0x1f83d9abU,0x5be0cd19U};
    unsigned i; for(i=0;i<8;i++) c->h[i]=iv[i]; c->total=0; c->used=0;
}
int ribat_sha256_update(RibatSha256 *c, const void *data, ribat_u64 len) {
    const ribat_u8 *p=(const ribat_u8 *)data;
    const ribat_u64 max_bytes=((ribat_u64)~0UL)>>3;
    if (len && !p) return -1;
    if (len > max_bytes-c->total) return -1;
    c->total += len;
    while (len) {
        ribat_u32 n=64U-c->used;
        if ((ribat_u64)n>len) n=(ribat_u32)len;
        for (ribat_u32 i=0;i<n;i++) c->block[c->used+i]=p[i];
        c->used+=n; p+=n; len-=n;
        if (c->used==64U) { compress(c,c->block); c->used=0; }
    }
    return 0;
}
void ribat_sha256_final(RibatSha256 *c, ribat_u8 out[32]) {
    ribat_u64 bits=c->total<<3;
    unsigned i;
    c->block[c->used++]=0x80;
    if (c->used>56U) { while(c->used<64U)c->block[c->used++]=0; compress(c,c->block); c->used=0; }
    while(c->used<56U)c->block[c->used++]=0;
    for(i=0;i<8;i++) c->block[63-i]=(ribat_u8)(bits>>(8*i));
    compress(c,c->block);
    for(i=0;i<8;i++) store_be32(out+4*i,c->h[i]);
    c->used=0;
}
int ribat_sha256(const void *data, ribat_u64 len, ribat_u8 out[32]) {
    RibatSha256 c; ribat_sha256_init(&c); if(ribat_sha256_update(&c,data,len)) return -1; ribat_sha256_final(&c,out); return 0;
}
