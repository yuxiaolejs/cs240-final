#ifndef TIMING_H
#define TIMING_H
#include "types.h"
static inline void cycle_cnt_init(void)
{
    uint32_t in = 1;
    asm volatile("MCR p15, 0, %0, c15, c12, 0" ::"r"(in));
}

// read cycle counter: should add a write().
static inline uint32_t cycle_cnt_read(void)
{
    uint32_t out;
    asm volatile("MRC p15, 0, %0, c15, c12, 1" : "=r"(out));
    return out;
}
#endif