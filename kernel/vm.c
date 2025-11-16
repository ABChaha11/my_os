#include "printf.h"
#include "pmm.h"
#include "riscv.h"
#include "memlayout.h"
#include "uart.h" // For UART0 address
#include "vm.h"
#include "defs.h"
#include "param.h" // For NPROC

// 内核页表
pagetable_t kernel_pagetable;

// 由链接器脚本提供
extern char etext[];  // 内核代码段 (.text) 的结束地址

// 内部辅助函数声明
static void dump_pagetable_recursive(pagetable_t pt, int level);


/**
 * @brief 查找一个虚拟地址对应的PTE，如果中间页表不存在且alloc=1，则创建。
 * @param pt 根页表的地址
 * @param va 要查找的虚拟地址
 * @param alloc 是否分配新页 (1 = 是, 0 = 否)
 * @return 成功则返回指向最末级PTE的指针，失败则返回0
 */
pte_t* walk(pagetable_t pt, uint64_t va, int alloc) {
    // Sv39 虚拟地址不能超过最大值
    if (va >= (1L << 39)) {
        panic("walk: va out of range");
    }

    // 从2级页表开始遍历
    for (int level = 2; level > 0; level--) {
        pte_t *pte = &pt[VPN_MASK(va, level)];

        if (*pte & PTE_V) {
            // PTE有效，直接进入下一级页表
            pt = (pagetable_t)PTE2PA(*pte);
        } else {
            // PTE无效
            if(!alloc || (pt = (pagetable_t)alloc_page()) == 0)
                return 0; // 不分配或内存不足
            
            // 将新分配的页地址填入PTE，并标记为有效
            *pte = PA2PTE(pt) | PTE_V;
        }
    }
    // 返回最末级 (level 0) PTE的地址
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
int kvmmap(pagetable_t pt, uint64_t va, uint64_t pa, uint64_t size, int perm) {
    uint64_t current_va = PGROUNDDOWN(va);
    uint64_t last_va = PGROUNDDOWN(va + size - 1);
    
    pte_t *pte;

    for (;;) {
        pte = walk(pt, current_va, 1); // 1 = alloc
        if (pte == 0) {
            return -1; // 内存分配失败
        }
        if (*pte & PTE_V) {
            printf("kvmmap: va 0x%lx pa 0x%lx\n", current_va, pa);
            panic("kvmmap: remap"); // 不允许重新映射
        }
        *pte = PA2PTE(pa) | perm | PTE_V;
        
        if (current_va == last_va) {
            break;
        }
        current_va += PGSIZE;
        pa += PGSIZE; // 注意: pa 也要递增
    }
    return 0;
}

/**
 * @brief 取消一段虚拟地址的映射
 * @param pt 页表
 * @param va 虚拟地址起始
 * @param size 大小
 * @param do_free 是否释放PTE指向的物理页 (1 = 是, 0 = 否)
 */
void kvmunmap(pagetable_t pt, uint64_t va, uint64_t size, int do_free) {
    uint64_t current_va = PGROUNDDOWN(va);
    uint64_t last_va = PGROUNDDOWN(va + size - 1);
    pte_t *pte;

    for (;;) {
        pte = walk(pt, current_va, 0); // 0 = no alloc
        if(pte == 0) {
            // 如果pte为0, 假设它已经被unmap了 (例如在kwait中)
            // panic("kvmunmap: pte not found"); 
        } else if((*pte & PTE_V) != 0) { // 仅当PTE有效时才操作
            if(do_free) {
                uint64_t pa = PTE2PA(*pte);
                free_page((void*)pa);
            }
            *pte = 0; // 清除PTE，使其无效
        }

        if (current_va == last_va)
            break;
        
        current_va += PGSIZE;
    }
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
    // 将页表清零
    for(int i=0; i<PGSIZE/sizeof(uint64_t); i++) kernel_pagetable[i] = 0;


    // 1. 映射 UART 设备 (R+W)
    kvmmap(kernel_pagetable, UART0, UART0, PGSIZE, PTE_R | PTE_W);
    printf("kvminit: mapped uart (0x%lx)\n", (uint64_t)UART0);

    // 2. 映射内核 .text 段 (R+X)
    kvmmap(kernel_pagetable, KERNBASE, KERNBASE, (uint64_t)etext - KERNBASE, PTE_R | PTE_X);
    printf("kvminit: mapped kernel text [0x%lx, 0x%lx)\n", KERNBASE, (uint64_t)etext);

    // 3. 映射 .rodata, .data, .bss 和剩余的 RAM (R+W)
    uint64_t rw_start = PGROUNDUP((uint64_t)etext);

    // --- 关键修复 ---
    // 计算内核栈区域的基地址 (即最低的栈的最低地址)
    // KSTACK(NPROC-1) 是最低栈的栈顶
    // kstack_base 是最低栈的栈底
    //uint64_t kstack_base = KSTACK(NPROC-1) - KSTACKSIZE;
    
    // 只映射到内核栈的底部，[kstack_base, PHYSTOP) 区域留给 proc_init
    //kvmmap(kernel_pagetable, rw_start, rw_start, kstack_base - rw_start, PTE_R | PTE_W);
    //printf("kvminit: mapped kernel data and ram [0x%lx, 0x%lx)\n", rw_start, kstack_base);
    
    // 我们必须恒等映射所有 RAM，从 etext 到 PHYSTOP。
    // 这片区域包括了 .data, .bss, pmm管理的空闲内存, 
    // 以及未来要分配给页表和内核栈的物理内存。
    kvmmap(kernel_pagetable, rw_start, rw_start, PHYSTOP - rw_start, PTE_R | PTE_W);
    printf("kvminit: mapped kernel data and ram [0x%lx, 0x%lx)\n", rw_start, PHYSTOP);
    
    
    // (内核栈将在 proc_init 中被映射到 [kstack_base, PHYSTOP) 区域)
    
    printf("kvminit: kernel page table created successfully.\n");
}

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
            printf("[%d] pte=0x%lx -> pa=0x%lx, flags=", i, pte, PTE2PA(pte));
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