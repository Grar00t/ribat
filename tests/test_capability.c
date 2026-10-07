#include "../src/capability.h"

static void fill(ribat_u8 *p, unsigned n, ribat_u8 seed) {
    unsigned i;
    for (i = 0; i < n; i++) p[i] = (ribat_u8)(seed + i);
}

int main(void) {
    ribat_u8 owner[16], resource[32];
    RibatCapTable table;
    RibatCapability root_cap, child_cap, delegated, replacement;
    unsigned root, child, reused;
    ribat_u64 child_version;

    fill(owner, sizeof owner, 0x10);
    fill(resource, sizeof resource, 0x80);
    if (ribat_cap_table_init(&table, owner, 7) != RIBAT_CAP_OK) return 1;
    if (ribat_cap_guard_create(&table, RIBAT_CAP_NO_PARENT, &root) != RIBAT_CAP_OK) return 2;
    if (ribat_cap_guard_create(&table, root, &child) != RIBAT_CAP_OK) return 3;
    if (ribat_cap_issue(&table, root, resource, RIBAT_CAP_READ | RIBAT_CAP_WRITE | RIBAT_CAP_DELEGATE,
                        &root_cap) != RIBAT_CAP_OK) return 4;
    if (ribat_cap_issue(&table, child, resource, RIBAT_CAP_READ, &child_cap) != RIBAT_CAP_OK) return 5;
    if (ribat_cap_validate(&table, &root_cap, RIBAT_CAP_WRITE) != RIBAT_CAP_OK) return 6;
    if (ribat_cap_validate(&table, &child_cap, RIBAT_CAP_READ) != RIBAT_CAP_OK) return 7;

    child_version = table.guards[child].version;
    if (ribat_cap_delegate(&root_cap, RIBAT_CAP_READ, &delegated) != RIBAT_CAP_OK) return 8;
    if (table.guards[child].version != child_version || !table.guards[child].live) return 9;
    if (ribat_cap_validate(&table, &delegated, RIBAT_CAP_READ) != RIBAT_CAP_OK) return 10;
    if (ribat_cap_validate(&table, &delegated, RIBAT_CAP_WRITE) != RIBAT_CAP_ERR_RIGHTS) return 11;
    if (ribat_cap_delegate(&root_cap, RIBAT_CAP_EXEC, &delegated) != RIBAT_CAP_ERR_RIGHTS) return 12;

    if (ribat_cap_guard_revoke(&table, child) != RIBAT_CAP_OK) return 13;
    if (ribat_cap_validate(&table, &child_cap, RIBAT_CAP_READ) != RIBAT_CAP_ERR_REVOKED) return 14;
    if (ribat_cap_validate(&table, &root_cap, RIBAT_CAP_READ) != RIBAT_CAP_OK) return 15;
    if (ribat_cap_guard_create(&table, root, &reused) != RIBAT_CAP_OK || reused != child) return 16;
    if (table.guards[reused].version != child_version + 1) return 17;
    if (ribat_cap_issue(&table, reused, resource, RIBAT_CAP_READ, &replacement) != RIBAT_CAP_OK) return 18;
    if (ribat_cap_validate(&table, &child_cap, RIBAT_CAP_READ) != RIBAT_CAP_ERR_REVOKED) return 19;
    if (ribat_cap_validate(&table, &replacement, RIBAT_CAP_READ) != RIBAT_CAP_OK) return 20;

    if (ribat_cap_guard_revoke(&table, root) != RIBAT_CAP_OK) return 21;
    if (ribat_cap_validate(&table, &root_cap, RIBAT_CAP_READ) != RIBAT_CAP_ERR_REVOKED) return 22;
    if (ribat_cap_validate(&table, &replacement, RIBAT_CAP_READ) != RIBAT_CAP_ERR_REVOKED) return 23;

    if (ribat_cap_guard_create(&table, RIBAT_CAP_NO_PARENT, &root) != RIBAT_CAP_OK) return 24;
    if (ribat_cap_issue(&table, root, resource, RIBAT_CAP_READ, &replacement) != RIBAT_CAP_OK) return 25;
    if (ribat_cap_owner_restart(&table) != RIBAT_CAP_OK) return 26;
    if (ribat_cap_validate(&table, &replacement, RIBAT_CAP_READ) != RIBAT_CAP_ERR_OWNER) return 27;

    table.owner_version = (ribat_u64)~0UL;
    if (ribat_cap_owner_restart(&table) != RIBAT_CAP_ERR_VERSION_EXHAUSTED) return 28;
    return 0;
}
