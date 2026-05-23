#ifndef THREADS
#define THREADS
#include "types.h"
#include "mem.h"

#define trace(fmt, ...) \
    printk("TRACE:%s:" fmt, __func__, ##__VA_ARGS__)
#define output trace

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



typedef struct
{
    uint32_t tid;
    uint32_t sp;
    uint8_t exited;
} rpi_thread_t;

typedef struct rpi_thread_fifo
{
    rpi_thread_t *t;
    struct rpi_thread_fifo *next;
} rpi_thread_fifo;

rpi_thread_t *rpi_cur_thread();
void rpi_thread_start();
void rpi_fork(void (*func)(void *), void *args_ptr);
void rpi_yield();
void rpi_exit(uint32_t n);

void sleep_ms(uint32_t ms);
void sleep_us(uint32_t us);
#endif