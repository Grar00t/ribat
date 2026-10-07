#ifndef RIBAT_CAPABILITY_H
#define RIBAT_CAPABILITY_H

#include "sha256.h"

#define RIBAT_CAP_GUARDS_MAX 64U
#define RIBAT_CAP_NO_PARENT 0xffffffffU

#define RIBAT_CAP_READ     0x01U
#define RIBAT_CAP_WRITE    0x02U
#define RIBAT_CAP_EXEC     0x04U
#define RIBAT_CAP_DELEGATE 0x08U
#define RIBAT_CAP_REVOKE   0x10U
#define RIBAT_CAP_RIGHTS_ALL 0x1fU

typedef struct {
    ribat_u64 version;
    unsigned parent;
    ribat_u8 live;
} RibatCapGuard;

typedef struct {
    ribat_u8 owner_id[16];
    ribat_u64 owner_version;
    RibatCapGuard guards[RIBAT_CAP_GUARDS_MAX];
} RibatCapTable;

typedef struct {
    ribat_u8 owner_id[16];
    ribat_u64 owner_version;
    unsigned guard_index;
    ribat_u64 guard_version;
    ribat_u8 resource_id[32];
    unsigned rights;
} RibatCapability;

enum {
    RIBAT_CAP_OK = 0,
    RIBAT_CAP_ERR_INPUT = -1,
    RIBAT_CAP_ERR_FULL = -2,
    RIBAT_CAP_ERR_OWNER = -3,
    RIBAT_CAP_ERR_GUARD = -4,
    RIBAT_CAP_ERR_REVOKED = -5,
    RIBAT_CAP_ERR_RIGHTS = -6,
    RIBAT_CAP_ERR_VERSION_EXHAUSTED = -7
};

int ribat_cap_table_init(RibatCapTable *table, const ribat_u8 owner_id[16], ribat_u64 owner_version);
int ribat_cap_owner_restart(RibatCapTable *table);
int ribat_cap_guard_create(RibatCapTable *table, unsigned parent, unsigned *guard_index);
int ribat_cap_guard_revoke(RibatCapTable *table, unsigned guard_index);
int ribat_cap_issue(const RibatCapTable *table, unsigned guard_index, const ribat_u8 resource_id[32],
                    unsigned rights, RibatCapability *out);
int ribat_cap_delegate(const RibatCapability *parent, unsigned rights, RibatCapability *out);
int ribat_cap_validate(const RibatCapTable *table, const RibatCapability *cap, unsigned required_rights);

#endif
