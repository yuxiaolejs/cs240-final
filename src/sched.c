#include "sched.h"
#include "cstr.h"
#include "pictl.h"
#include "instrument.h"

static rpi_thread_t root_t;
rpi_thread_fifo *ready_q = 0;
rpi_thread_t *cur = &root_t;
#define INIT_TSTACK_ADDR 0x110000
#define INCR_TSTACK_ADDR 0x10000

uint32_t last_stack_addr = INIT_TSTACK_ADDR;
uint32_t tid = 1;

uint32_t last_cs_tick;

__attribute__((naked)) void t_cs(rpi_thread_t *old, rpi_thread_t *next)
{
    asm volatile(
        "stmdb sp!, {r0-r12, lr}    \n"
        "mrs   r2, cpsr             \n"
        "stmdb sp!, {r2}            \n"
        "str   sp, [r0, #%c0]       \n"
        "ldr   sp, [r1, #%c0]       \n"
        "ldmia sp!, {r2}            \n"
        "msr   cpsr_f, r2         \n"
        "ldmia sp!, {r0-r12, lr}    \n"
        "bx    lr                   \n"
        :
        : "i"(4)
        : "memory");
}

rpi_thread_t *t_pop()
{
    if (ready_q)
    {
        rpi_thread_t *t = ready_q->t;
        rpi_thread_fifo *old_head = ready_q;
        ready_q = ready_q->next; // memory leak!
        kfree(old_head);
        return t;
    }
    return 0;
}
void t_push(rpi_thread_t *x)
{
    rpi_thread_fifo *o = kalloc(sizeof(rpi_thread_fifo));
    o->next = 0;
    o->t = x;
    if (!ready_q)
    {
        ready_q = o;
        return;
    }
    rpi_thread_fifo *c = ready_q;
    while (c->next)
        c = c->next;
    c->next = o;
}
void rpi_yield()
{
    uint32_t current_tick = get_current_tick();
    if (current_tick - last_cs_tick > 2){
        printk(" too fucking slow: %d, from %d\n", current_tick - last_cs_tick, cur->tid);
    }
    last_cs_tick = get_current_tick();
    rpi_thread_t *next = t_pop();
    if (next)
    {
        if (!cur->exited && cur != &root_t)
        {
            t_push(cur);
        }
        rpi_thread_t *old = cur;
        cur = next;
        t_cs(old, next);
    }
}

rpi_thread_t *rpi_cur_thread()
{
    return cur;
}
void rpi_thread_start()
{
    trace("starting threads!\n");
    rpi_yield();
    // trace("Came back from first huh\n");
    rpi_thread_t *next = t_pop();
    while (next)
        t_cs(cur, next);
    trace("done with all threads, returning\n");
}
void rpi_exit(uint32_t x)
{
    // trace("rpi exit\n");
    cur->exited = true;
    rpi_yield();
    if (!x)
        trace("done running threads, back to scheduler\n");
    rpi_thread_t *old = cur;
    cur = &root_t;
    t_cs(old, &root_t);
    trace("SHOULD NOT BE REACHABLE\n\n");
    rpi_reboot();
}
void thread_wrap(void (*func)(void *), void *args_ptr)
{
    func(args_ptr);
    cur->exited = true;
    rpi_exit(1);
}
void rpi_fork(void (*func)(void *), void *args_ptr)
{
    rpi_thread_t *t = kalloc(sizeof(rpi_thread_t));
    uint32_t *sp = (uint32_t *)last_stack_addr;
    *--sp = (uint32_t)thread_wrap;
    for (int i = 12; i >= 2; i--)
        *--sp = 0;
    *--sp = (uint32_t)args_ptr;
    *--sp = (uint32_t)func;

    uint32_t cpsr;
    asm volatile("mrs %0, cpsr" : "=r"(cpsr));
    *--sp = cpsr;

    t->sp = (uint32_t)sp;
    t->tid = tid++;
    t->exited = 0;
    last_stack_addr += INCR_TSTACK_ADDR;
    trace("rpi_fork: tid=%d, code=\n", t->tid);
    t_push(t);
}
void yield_until_ddl(uint32_t deadline)
{
    while (cycle_cnt_read() < deadline)
        rpi_yield();
}

void sleep_ms(uint32_t ms)
{
    cycle_cnt_init();
    uint32_t start = cycle_cnt_read();
    uint32_t cycles_to_wait = ms * 700000; // Assuming a 700MHz CPU
    uint32_t deadline = start + cycles_to_wait;
    yield_until_ddl(deadline);
}

void sleep_us(uint32_t us)
{
    uint32_t end_time = timer_get_usec_raw() + us;
    while (timer_get_usec_raw() < end_time)
    {
    }
}
