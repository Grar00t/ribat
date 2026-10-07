#ifndef RIBAT_OBJECT_ID_H
#define RIBAT_OBJECT_ID_H
#include "sha256.h"
int ribat_object_id(const char type[3], unsigned version, const void *payload, ribat_u64 payload_len, ribat_u8 out[32]);
#endif
