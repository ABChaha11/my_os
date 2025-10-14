#include "printf.h"
#include "memlayout.h"
#include "riscv.h"
#include "pmm.h"
#include "defs.h"

// 由链接器脚本 kernel.ld 定义，代表内核数据段结束后的第一个地址
extern char _end[];

// 空闲物理页的链表节点结构
// 它巧妙地利用了空闲页面自身的空间来存储下一个节点的地址
struct page_info {
    struct page_info *next;
};

// 指向空闲链表的头部
static struct page_info *free_list_head;

/**
 * @brief 初始化物理内存管理器
 * 这个函数会扫描从内核末尾到物理内存顶部的所有内存，
 * 并将所有可用的物理页加入到空闲链表中。
 */
void pmm_init() {
    printf("pmm: initializing...\n");
    free_list_head = 0; // 初始化为空链表
    
    // 从 _end 向上对齐到页面边界开始管理
    char *p = (char*)PGROUNDUP((uint64_t)_end);
    printf("pmm: managing memory from 0x%lx to 0x%lx\n", (uint64_t)p, (uint64_t)PHYSTOP);

    // 逐页将可用物理内存加入空闲链表
    for (; (uint64_t)p + PGSIZE <= (uint64_t)PHYSTOP; p += PGSIZE) {
        free_page(p);
    }
    printf("pmm: initialization complete.\n");
}

/**
 * @brief 释放一个物理页
 * @param pa 要释放的物理页的起始地址
 */
void free_page(void *pa) {
    // 错误检查: 地址不能为NULL, 必须页对齐, 且在PMM的管理范围内
    if (pa == 0 || (uint64_t)pa % PGSIZE != 0 || (uint64_t)pa < (uint64_t)_end || (uint64_t)pa >= (uint64_t)PHYSTOP) {
        panic("free_page: invalid physical address");
    }

    // 将页面内容填充为特定值，有助于调试 use-after-free 问题
    // for(int i=0; i<PGSIZE; i++) ((char*)pa)[i] = 0xcc;

    struct page_info *page = (struct page_info *)pa;

    // 头插法将页面加入空闲链表
    page->next = free_list_head;
    free_list_head = page;
}

/**
 * @brief 分配一个物理页
 * @return 成功则返回物理页地址，失败 (内存耗尽) 则返回0
 */
void* alloc_page(void) {
    // 如果空闲链表为空，则物理内存耗尽
    if (free_list_head == 0) {
        return 0;
    }

    // 从链表头取出一个页面
    struct page_info *page = free_list_head;
    free_list_head = page->next;
    
    // 将页面内容清零，这是一个好的实践
    for(int i=0; i<PGSIZE; i++) ((char*)page)[i] = 0;

    return (void*)page;
}