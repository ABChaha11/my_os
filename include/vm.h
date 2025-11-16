#ifndef __VM_H__
#define __VM_H__

#include "types.h"

void kvminit(void);
void kvminithart(void);
pte_t* walk(pagetable_t pt, uint64_t va, int alloc);
int kvmmap(pagetable_t pt, uint64_t va, uint64_t pa, uint64_t size, int perm);
void kvmunmap(pagetable_t pt, uint64_t va, uint64_t size, int do_free);
void dump_pagetable(pagetable_t pt);

#endif