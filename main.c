// from ch3 of the arm1176.pdf manual in doc/

// https://www.songstuff.com/recording/article/music-fundamental-frequencies/
#include "instrument.h"
#include "song.h"
#include "mem.h"
#include "midi.h"
#include "regs.h"
#include "p10.h"
#include "spi.h"
#include "minispi.h"
#include "osc.h"
#include "timer.h"
#include "dma.h"
#include "sched.h"
#define MIDI_PIN 26

void main_loop();
typedef struct instrument_list
{
    instrument_t *inst;
    struct instrument_list *next;
} instrument_list;

extern volatile uint32_t _interrupt_table[];

static instrument_list *main_head;

void interrupts_table_init(void)
{
    asm volatile("mcr p15, 0, %0, c12, c0, 0" : : "r"(_interrupt_table) : "memory");
    // enable irq
    asm volatile("cpsie i" : : : "memory");
}

void irq_handler()
{
    if (GET32(ADD_IRQ_PENDING2) & (1 << 17))
    {
        uint32_t cpsr = 0;
        asm volatile("mrs %0, cpsr" : "=r"(cpsr));
        midi_event ev = midi_scan(MIDI_PIN);
        while (ev.channel != -1)
        {
            // printk("MIDI %d: %d Hz\n", ev.channel, ev.freq);
            uint8_t shift = ev.channel;
            instrument_list *head = main_head;
            while (head && shift--)
                head = head->next;
            if (head)
            {
                head->inst->period = TICK_RATE / ev.freq;
                // printk("%d: %d freq: %d, duration: %d\n", last_update_idx, current_note.channel, current_note.freq, current_note.duration_ms);
            }
            ev = midi_scan(MIDI_PIN);
        }
        PUT32(ADD_GPEDS, 0xFFFFFFFF);
    }
    else
    {
        MK_REG(0x2000B40C) = 1;
    }
}

instrument_list *append_instrument(instrument_list *head, instrument_t *inst)
{
    instrument_list *new_node = (instrument_list *)kalloc(sizeof(instrument_list));
    new_node->inst = inst;
    new_node->next = NULL;
    if (!head)
    {
        head = new_node;
        return head;
    }
    instrument_list *cur = head;
    while (cur->next)
        cur = cur->next;
    cur->next = new_node;
    return head;
}

uint32_t last_update_tick = 0;
uint32_t last_update_idx = 0;

inline void update_instruments_relative_tick(instrument_list *head)
{
    // real time midi play
    return;
    midi_event ev = midi_scan(MIDI_PIN);
    printk("MIDI %d: %d Hz\n", ev.channel, ev.freq);
    uint8_t shift = ev.channel + 1;
    while (head && shift--)
        head = head->next;
    if (head && (ev.freq > 270 || ev.freq == 0))
    {
        head->inst->period = TICK_RATE / ev.freq;
        // printk("%d: %d freq: %d, duration: %d\n", last_update_idx, current_note.channel, current_note.freq, current_note.duration_ms);
    }
    return;
    // below is playing from recorded source
    uint32_t tick = get_current_tick();
    note_t current_note = song[last_update_idx];
    while ((int64_t)tick - (int64_t)last_update_tick >= current_note.delay_ms * (TICK_RATE / 1000))
    {

        last_update_idx++;
        uint8_t shift = current_note.channel;
        while (head && shift--)
            head = head->next;
        if (head && (current_note.freq > 270 || current_note.freq == 0))
        {
            head->inst->period = TICK_RATE / current_note.freq;
            // printk("%d: %d freq: %d, duration: %d\n", last_update_idx, current_note.channel, current_note.freq, current_note.duration_ms);
        }
        if (last_update_idx >= song_len)
        {
            last_update_idx = 0; // loop the song
        }
        current_note = song[last_update_idx];
        last_update_tick += current_note.delay_ms * (TICK_RATE / 1000);
        tick = get_current_tick();
    }
}

void main_loop()
{
    while (1)
    {
        instrument_list *cur = main_head;
        // update_instruments_relative_tick(main_head);
        while (cur)
        {
            tick_instrument(cur->inst);
            cur = cur->next;
        }
        rpi_yield();
    }
}

instrument_list *make_floppy_inst(instrument_list *head, uint32_t gpio_primary, uint32_t gpio_secondary)
{
    instrument_t *inst = (instrument_t *)kalloc(sizeof(instrument_t));
    memset(inst, 0, sizeof(instrument_t));
    inst->type = INSTRUMENT_TYPE_FLOPPY;
    io_dev_t *dev = (io_dev_t *)kalloc(sizeof(io_dev_t));
    dev->gpio_primary = gpio_primary;
    dev->gpio_secondary = gpio_secondary;
    inst->dev = dev;
    return append_instrument(head, inst);
}

instrument_list *make_hbridge_inst(instrument_list *head, uint32_t gpio_primary, uint32_t gpio_secondary)
{
    instrument_t *inst = (instrument_t *)kalloc(sizeof(instrument_t));
    memset(inst, 0, sizeof(instrument_t));
    inst->type = INSTRUMENT_TYPE_HBRIDGE;
    io_dev_t *dev = (io_dev_t *)kalloc(sizeof(io_dev_t));
    dev->gpio_primary = gpio_primary;
    dev->gpio_secondary = gpio_secondary;
    inst->dev = dev;
    return append_instrument(head, inst);
}

void notmain(void)
{
    enable_vfp();
    printk("Starting run ");
    printk("\n");
    kalloc_init();
    uint32_t *ptable = mmu_init();
    printk("MMU ON\n");
    gpio_set_input(MIDI_PIN);
    gpio_set_pullup(MIDI_PIN);
    spi_init();
    interrupts_table_init();
    minispi_init();
    w5500_init();
    main_head = NULL;
    // head = make_floppy_inst(head, 24, 23);
    // main_head = make_floppy_inst(main_head, 0, 1);
    // main_head = make_hbridge_inst(main_head, 2, 3);
    // main_head = make_hbridge_inst(main_head, 4, 5);
    // main_head = make_hbridge_inst(main_head, 6, 7);
    // main_head = make_hbridge_inst(main_head, 8, 9);
    // main_head = make_hbridge_inst(main_head, 12, 13);
    // head = make_floppy_inst(head, 20, 21);
    // head = make_floppy_inst(head, 19, 26);

    enable_timer(1, TIMER_PRE_16); // 1000 * 256 / 250000000 = 1.024 ms per tick

    *PTR_IRQ_ENABLE2 = (1u << 49 - 32); // enable GPIO IRQ
    // *PTR_IRQ_ENABLE_BASIC |= 1;         // timer
    *PTR_GPFEN0 |= (1 << MIDI_PIN);     // enable falling edge detect for MIDI pin
    rpi_fork(p10_start_server, NULL);
    // p10_start_server();
    rpi_fork(main_loop, NULL);
    rpi_thread_start();
    rpi_reboot();
    // recv_main(); // Or run this
}