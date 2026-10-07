#include "../src/sha256.h"
#include "../src/object_id.h"
static int eq(const ribat_u8 *a,const ribat_u8 *b,unsigned n){unsigned i;for(i=0;i<n;i++)if(a[i]!=b[i])return 0;return 1;}
static int hexv(char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;return -1;}
static int hex32(const char *s,ribat_u8 out[32]){unsigned i;for(i=0;i<32;i++){int a=hexv(s[2*i]),b=hexv(s[2*i+1]);if(a<0||b<0)return -1;out[i]=(ribat_u8)((a<<4)|b);}return 0;}
static int kat(const void *p,ribat_u64 n,const char *hex){ribat_u8 got[32],want[32];if(hex32(hex,want)||ribat_sha256(p,n,got))return 0;return eq(got,want,32);}
static int run(void){
    static const char abc[]="abc";
    static const char longmsg[]="abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    ribat_u8 got[32], want[32];
    if(!kat("",0,"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"))return 1;
    if(!kat(abc,3,"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"))return 2;
    if(!kat(longmsg,sizeof longmsg-1,"248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"))return 3;
    if(ribat_object_id("DAT",1,abc,3,got))return 4;
    if(hex32("ae4dfa54323e48832c614e292c0562cb116ccce59cf3fa78c59f3f8be9236d01",want))return 5;
    if(!eq(got,want,32))return 6;
    return 0;
}
int main(void){return run();}
