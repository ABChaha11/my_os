#include "printf.h"
#include "pmm.h"
#include "riscv.h"
#include "memlayout.h"
#include "uart.h" // For UART0 address
#include "vm.h"
#include "defs.h"

// 内核页表
pagetable_t kernel_pagetable;

// 由链接器脚本提供
extern char etext[];  // 内核代码段 (.text) 的结束地址
extern char erodata[];  // .rodata 段的结束

// 内部辅助函数声明
static pte_t* walk_create(pagetable_t pt, uint64_t va);
static void dump_pagetable_recursive(pagetable_t pt, int level);
static int map_region(pagetable_t pt, uint64_t va, uint64_t pa, uint64_t size, int perm);


/**
 * @brief 查找一个虚拟地址对应的PTE，如果中间页表不存在则创建。
 * @param pt 根页表的地址
 * @param va 要查找的虚拟地址
 * @return 成功则返回指向最末级PTE的指针，失败 (内存分配失败) 则返回0
 */
static pte_t* walk_create(pagetable_t pt, uint64_t va) {
    // Sv39 虚拟地址不能超过最大值
    if (va >= (1L << 39)) {
        panic("walk_create: va out of range");
    }

    // 从2级页表开始遍历
    for (int level = 2; level > 0; level--) {
        pte_t *pte = &pt[VPN_MASK(va, level)];

        if (*pte & PTE_V) {
            // PTE有效，直接进入下一级页表
            pt = (pagetable_t)PTE2PA(*pte);
        } else {
            // PTE无效，需要分配一个新的页表页
            pt = (pagetable_t)alloc_page();
            if (pt == 0) {
                // 物理内存不足
                return 0;
            }
            // 将新分配的页地址填入PTE，并标记为有效
            *pte = PA2PTE(pt) | PTE_V;
        }
    }
    // 返回最末级 (level 0) PTE的地址
    return &pt[VPN_MASK(va, 0)];
}

/**
 * @brief 查找一个虚拟地址对应的PTE，只查找，不创建。
 * @param pt 根页表的地址
 * @param va 要查找的虚拟地址
 * @return 成功则返回指向最末级PTE的指针，如果未映射则返回0
 */
pte_t* walk_lookup(pagetable_t pt, uint64_t va) {
    if (va >= (1L << 39)) {
        return 0;
    }

    for (int level = 2; level > 0; level--) {
        pte_t *pte = &pt[VPN_MASK(va, level)];
        if (*pte & PTE_V) {
            pt = (pagetable_t)PTE2PA(*pte);
        } else {
            return 0; // 中间页表不存在，映射肯定不存在
        }
    }
    return &pt[VPN_MASK(va, 0)];
}

/**
 * @brief 在一个页表中映射一段连续的虚拟地址到物理地址
 * @param pt 页表
 * @param va 虚拟地址起始
 * @param pa 物理地址起始
 * @param size 映射大小
 * @param perm 权限位
 * @return 成功返回0, 失败返回-1
 */
static int map_region(pagetable_t pt, uint64_t va, uint64_t pa, uint64_t size, int perm) {
    /* --- FIX: 将起始地址和结束地址都进行页对齐 --- */
    uint64_t current_va = PGROUNDDOWN(va);
    uint64_t last_va = PGROUNDDOWN(va + size - 1);
    
    // 物理地址也需要对齐
    uint64_t current_pa = PGROUNDDOWN(pa);

    for (;;) {
        if (map_page(pt, current_va, current_pa, perm) != 0) {
            return -1; // 映射失败
        }
        if (current_va == last_va) {
            break;
        }
        current_va += PGSIZE;
        current_pa += PGSIZE;
    }
    return 0;
}

/**
 * @brief 在页表中创建一个虚拟地址到物理地址的映射
 * @param pt 页表
 * @param va 虚拟地址 (必须页对齐)
 * @param pa 物理地址 (必须页对齐)
 * @param perm 权限位 (PTE_R, PTE_W, PTE_X)
 * @return 成功返回0，失败返回-1
 */
int map_page(pagetable_t pt, uint64_t va, uint64_t pa, int perm) {
    if (va % PGSIZE != 0 || pa % PGSIZE != 0) {
        panic("map_page: address not page aligned");
    }

    pte_t *pte = walk_create(pt, va);
    if (pte == 0) {
        return -1; // 内存分配失败
    }
    if (*pte & PTE_V) {
        panic("map_page: remap"); // 不允许重新映射
    }
    *pte = PA2PTE(pa) | perm | PTE_V;
    return 0;
}

/**
 * @brief 创建并初始化内核页表
 */
void kvminit(void) {
    printf("kvminit: creating kernel page table...\n");

    kernel_pagetable = (pagetable_t)alloc_page();
    if (kernel_pagetable == 0) {
        panic("kvminit: out of memory for page table");
    }

    // 1. 映射 UART 设备 (R+W)
    map_region(kernel_pagetable, UART0, UART0, PGSIZE, PTE_R | PTE_W);
    printf("kvminit: mapped uart (0x%lx)\n", (uint64_t)UART0);

    // 2. 映射内核 .text 段 (R+X)
    map_region(kernel_pagetable, KERNBASE, KERNBASE, (uint64_t)etext - KERNBASE, PTE_R | PTE_X);
    printf("kvminit: mapped kernel text [0x%lx, 0x%lx)\n", KERNBASE, (uint64_t)etext);

    // 3. 映射 .rodata, .data, .bss 和剩余的 RAM (R+W)
    //    为了避免与 .text 段的最后一页重叠，我们从 etext 向上取整的页面开始映射。
    uint64_t rw_start = PGROUNDUP((uint64_t)etext);
    map_region(kernel_pagetable, rw_start, rw_start, PHYSTOP - rw_start, PTE_R | PTE_W);
    printf("kvminit: mapped kernel data and ram [0x%lx, 0x%lx)\n", rw_start, PHYSTOP);
    
    printf("kvminit: kernel page table created successfully.\n");}

/**
 * @brief 激活内核页表
 * 将内核页表的地址写入 satp 寄存器，并刷新 TLB
 */
void kvminithart(void) {
    w_satp(MAKE_SATP(kernel_pagetable));
    sfence_vma();
    printf("kvminithart: virtual memory enabled.\n");
}


/**
 * @brief 调试函数：打印页表内容 (递归实现)
 */
static void dump_pagetable_recursive(pagetable_t pt, int level) {
    if (level < 0) return;

    for (int i = 0; i < 512; i++) {
        pte_t pte = pt[i];
        if (pte & PTE_V) {
            for(int j=0; j< (2-level)*2; j++) printf("  "); // 缩进
            printf("[%d] pte=0x%lx -> pa=0x%lx, flags=", pte, PTE2PA(pte));
            if(pte & PTE_V) printf(" V");
            if(pte & PTE_R) printf(" R");
            if(pte & PTE_W) printf(" W");
            if(pte & PTE_X) printf(" X");
            if(pte & PTE_U) printf(" U");
            printf("\n");

            // 如果PTE不是叶子节点，则递归打印下一级页表
            if ((PTE_FLAGS(pte) & (PTE_R|PTE_W|PTE_X)) == 0) {
                dump_pagetable_recursive((pagetable_t)PTE2PA(pte), level - 1);
            }
        }
    }
}

/**
 * @brief 调试函数：打印页表的入口
 */
void dump_pagetable(pagetable_t pt) {
    printf("--- Page Table Dump ---\n");
    dump_pagetable_recursive(pt, 2); // 从 level 2 开始
    printf("-----------------------\n");
}