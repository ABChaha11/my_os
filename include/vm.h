#ifndef __VM_H__
#define __VM_H__

#include "types.h"

void kvminit(void);
void kvminithart(void);
int map_page(pagetable_t pt, uint64_t va, uint64_t pa, int perm);
pte_t* walk_lookup(pagetable_t pt, uint64_t va);
void dump_pagetable(pagetable_t pt);

#endif