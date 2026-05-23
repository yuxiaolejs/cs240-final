#ifndef VM_H
#define VM_H
#include "mem.h"

#define USER_SPACE_L1_INDEX_START 1024
#define USER_SPACE_L1_INDEX_END 1040
#define DCACHE_LINE_SIZE 32

extern void clean_invalidate_dcache(void);
extern void dcache_clean_invalidate_mva(void *addr);

static inline void clean_dcache_range(uintptr_t addr, size_t size)
{
    uintptr_t start = addr & ~(DCACHE_LINE_SIZE - 1);
    uintptr_t end = (addr + size + DCACHE_LINE_SIZE - 1) & ~(DCACHE_LINE_SIZE - 1);

    for (uintptr_t p = start; p < end; p += DCACHE_LINE_SIZE)
        dcache_clean_mva((void *)p);
}

uint32_t *mmu_init();
uint32_t *make_initial_L1_table();
void switch_L2_page_table(uint32_t *l1_base,
                          uint32_t l2_base,
                          uint32_t va);
void switch_L1_page_table(uint32_t pt_base, uint32_t context_id);
static inline void clean_dcache_ptes(void *pt_base, uint32_t size)
{
    uint32_t start = (uint32_t)pt_base & ~31;
    uint32_t end = (uint32_t)pt_base + size;

    for (uint32_t p = start; p < end; p += 32)
        dcache_clean_mva((void *)p);
    dsb();
}
inline void invalidate_for_l2_entry_swap(uint32_t l2_base, uint32_t va);
inline void code_onload_cache_sync(void *addr, uint32_t size);

uint32_t *fork_page_table(uint32_t *l1_table);
uint32_t *make_sure_user_space(uint32_t *l1_table_root, uint32_t user_va_start, uint32_t size); // Make sure user space is mapped and allocated
void free_L1_page_table(uint32_t *l1_table);
static inline void set_tls_base(void *tls)
{
    asm volatile(
        "mcr p15, 0, %0, c13, c0, 3"
        :
        : "r"(tls)
        : "memory");
}

static inline void code_onload_cache_sync(void *addr, uint32_t size)
{
    dsb();
    clean_dcache_range((uintptr_t)addr, size);
    dsb();
    invalidate_icache();
    isb();
    flush_branch_predictor();
    isb();
}
static inline void set_ttbr0(uint32_t ttbr0)
{
    asm volatile("mcr p15, 0, %0, c2, c0, 0" : : "r"(ttbr0) : "memory");
}
static inline void set_asid(uint32_t asid)
{
    asm volatile("mcr p15, 0, %0, c13, c0, 1" : : "r"(asid) : "memory");
}
static inline void set_context_id(uint32_t context_id)
{
    asm volatile("mcr p15, 0, %0, c13, c0, 0" : : "r"(context_id) : "memory");
}
static inline void flush_btb(void)
{
    asm volatile("mcr p15, 0, %0, c7, c5, 6" : : "r"(0) : "memory");
}
// static inline void switch_L1_page_table(uint32_t new_ttbr0, uint32_t new_context_id)
// {
//     uint32_t *new_l1_base = (uint32_t *)(new_ttbr0 & ~0x3FFF);
    
//     clean_dcache_ptes(new_l1_base, 16384);
//     dsb();

//     set_asid(0);
//     isb();

//     set_ttbr0(new_ttbr0);
//     isb();

//     set_context_id(new_context_id); // Not working since using v5 mode
//     isb();

//     flush_tlb();
//     isb();
//     invalidate_icache();
//     dsb();
//     flush_btb();
//     dsb();
//     isb();
// }
#endif