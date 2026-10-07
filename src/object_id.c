#include "object_id.h"
int ribat_object_id(const char type[3], unsigned version, const void *payload, ribat_u64 payload_len, ribat_u8 out[32]) {
    static const ribat_u8 domain[]={'R','I','B','A','T','/','O','B','J','/','1',0};
    ribat_u8 v[2], n[8]; RibatSha256 c; unsigned i;
    if (!type || !out || (payload_len && !payload) || version>0xffffU) return -1;
    v[0]=(ribat_u8)(version>>8); v[1]=(ribat_u8)version;
    for(i=0;i<8;i++) n[7-i]=(ribat_u8)(payload_len>>(8*i));
    ribat_sha256_init(&c);
    if(ribat_sha256_update(&c,domain,sizeof domain) || ribat_sha256_update(&c,type,3) ||
       ribat_sha256_update(&c,v,2) || ribat_sha256_update(&c,n,8) ||
       ribat_sha256_update(&c,payload,payload_len)) return -1;
    ribat_sha256_final(&c,out); return 0;
}
