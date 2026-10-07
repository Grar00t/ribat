#include "capability.h"

static void bytes_copy(ribat_u8 *dst, const ribat_u8 *src, unsigned n) {
    unsigned i;
    for (i = 0; i < n; i++) dst[i] = src[i];
}

static int bytes_equal(const ribat_u8 *a, const ribat_u8 *b, unsigned n) {
    unsigned i;
    ribat_u8 diff = 0;
    for (i = 0; i < n; i++) diff = (ribat_u8)(diff | (ribat_u8)(a[i] ^ b[i]));
    return diff == 0;
}

static int rights_valid(unsigned rights) {
    return rights != 0U && (rights & ~RIBAT_CAP_RIGHTS_ALL) == 0U;
}

int ribat_cap_table_init(RibatCapTable *table, const ribat_u8 owner_id[16], ribat_u64 owner_version) {
    unsigned i;
    if (!table || !owner_id || owner_version == 0) return RIBAT_CAP_ERR_INPUT;
    for (i = 0; i < 16; i++) table->owner_id[i] = owner_id[i];
    table->owner_version = owner_version;
    for (i = 0; i < RIBAT_CAP_GUARDS_MAX; i++) {
        table->guards[i].version = 0;
        table->guards[i].parent = RIBAT_CAP_NO_PARENT;
        table->guards[i].live = 0;
    }
    return RIBAT_CAP_OK;
}

int ribat_cap_owner_restart(RibatCapTable *table) {
    unsigned i;
    if (!table) return RIBAT_CAP_ERR_INPUT;
    if (table->owner_version == (ribat_u64)~0UL) return RIBAT_CAP_ERR_VERSION_EXHAUSTED;
    table->owner_version++;
    for (i = 0; i < RIBAT_CAP_GUARDS_MAX; i++) {
        table->guards[i].live = 0;
        table->guards[i].parent = RIBAT_CAP_NO_PARENT;
    }
    return RIBAT_CAP_OK;
}

int ribat_cap_guard_create(RibatCapTable *table, unsigned parent, unsigned *guard_index) {
    unsigned i;
    if (!table || !guard_index) return RIBAT_CAP_ERR_INPUT;
    if (parent != RIBAT_CAP_NO_PARENT &&
        (parent >= RIBAT_CAP_GUARDS_MAX || !table->guards[parent].live)) return RIBAT_CAP_ERR_GUARD;
    for (i = 0; i < RIBAT_CAP_GUARDS_MAX; i++) {
        RibatCapGuard *g = &table->guards[i];
        if (g->live) continue;
        if (g->version == (ribat_u64)~0UL) continue;
        g->version = g->version == 0 ? 1 : g->version + 1;
        g->parent = parent;
        g->live = 1;
        *guard_index = i;
        return RIBAT_CAP_OK;
    }
    for (i = 0; i < RIBAT_CAP_GUARDS_MAX; i++) {
        if (!table->guards[i].live && table->guards[i].version == (ribat_u64)~0UL)
            return RIBAT_CAP_ERR_VERSION_EXHAUSTED;
    }
    return RIBAT_CAP_ERR_FULL;
}

int ribat_cap_guard_revoke(RibatCapTable *table, unsigned guard_index) {
    unsigned pass, i;
    if (!table) return RIBAT_CAP_ERR_INPUT;
    if (guard_index >= RIBAT_CAP_GUARDS_MAX || !table->guards[guard_index].live)
        return RIBAT_CAP_ERR_GUARD;
    table->guards[guard_index].live = 0;
    for (pass = 0; pass < RIBAT_CAP_GUARDS_MAX; pass++) {
        int changed = 0;
        for (i = 0; i < RIBAT_CAP_GUARDS_MAX; i++) {
            unsigned p;
            if (!table->guards[i].live) continue;
            p = table->guards[i].parent;
            if (p != RIBAT_CAP_NO_PARENT && p < RIBAT_CAP_GUARDS_MAX && !table->guards[p].live) {
                table->guards[i].live = 0;
                changed = 1;
            }
        }
        if (!changed) break;
    }
    return RIBAT_CAP_OK;
}

int ribat_cap_issue(const RibatCapTable *table, unsigned guard_index, const ribat_u8 resource_id[32],
                    unsigned rights, RibatCapability *out) {
    if (!table || !resource_id || !out || !rights_valid(rights)) return RIBAT_CAP_ERR_INPUT;
    if (guard_index >= RIBAT_CAP_GUARDS_MAX || !table->guards[guard_index].live)
        return RIBAT_CAP_ERR_GUARD;
    bytes_copy(out->owner_id, table->owner_id, 16);
    out->owner_version = table->owner_version;
    out->guard_index = guard_index;
    out->guard_version = table->guards[guard_index].version;
    bytes_copy(out->resource_id, resource_id, 32);
    out->rights = rights;
    return RIBAT_CAP_OK;
}

int ribat_cap_delegate(const RibatCapability *parent, unsigned rights, RibatCapability *out) {
    if (!parent || !out || !rights_valid(rights)) return RIBAT_CAP_ERR_INPUT;
    if ((parent->rights & RIBAT_CAP_DELEGATE) == 0U) return RIBAT_CAP_ERR_RIGHTS;
    if ((rights & parent->rights) != rights) return RIBAT_CAP_ERR_RIGHTS;
    *out = *parent;
    out->rights = rights;
    return RIBAT_CAP_OK;
}

int ribat_cap_validate(const RibatCapTable *table, const RibatCapability *cap, unsigned required_rights) {
    const RibatCapGuard *g;
    if (!table || !cap || !rights_valid(required_rights)) return RIBAT_CAP_ERR_INPUT;
    if (!bytes_equal(table->owner_id, cap->owner_id, 16) || table->owner_version != cap->owner_version)
        return RIBAT_CAP_ERR_OWNER;
    if (cap->guard_index >= RIBAT_CAP_GUARDS_MAX) return RIBAT_CAP_ERR_GUARD;
    g = &table->guards[cap->guard_index];
    if (!g->live || g->version != cap->guard_version) return RIBAT_CAP_ERR_REVOKED;
    if ((cap->rights & required_rights) != required_rights) return RIBAT_CAP_ERR_RIGHTS;
    return RIBAT_CAP_OK;
}
