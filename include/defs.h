#ifndef __DEFS_H__
#define __DEFS_H__

// 内核全局函数声明

// from main.c
void panic(char *s);

// from trap.c
void trap_init(void);
void trapinithart(void);

// from vm.c
void kvminit(void);
void kvminithart(void);
pte_t* walk_lookup(pagetable_t pt, uint64_t va);

// from pmm.c
void pmm_init(void);
void* alloc_page(void);
void free_page(void* pa);

#endif