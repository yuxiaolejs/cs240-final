#ifndef INSTRUMENT_H
#define INSTRUMENT_H
#include "types.h"

typedef enum instrument_type
{
    INSTRUMENT_TYPE_FLOPPY,
    INSTRUMENT_TYPE_HBRIDGE,
    INSTRUMENT_TYPE_SPEAKER,
    INSTRUMENT_TYPE_PULSER,
};

typedef struct
{
    uint8_t gpio_primary;
    uint8_t gpio_secondary;
} io_dev_t;

typedef struct
{
    enum instrument_type type;
    io_dev_t *dev;
    uint32_t start_tick;
    uint32_t last_tick;
    uint32_t period;
    uint32_t state[10];
} instrument_t;

typedef struct
{
    enum instrument_type type;
    io_dev_t *dev;
    uint32_t start_tick;
    uint32_t last_tick;
    uint32_t period;
    uint32_t last_step_state;
    uint32_t last_dir_state;
    uint32_t dir_counter;

} floppy_instrument_t;

typedef struct
{
    enum instrument_type type;
    io_dev_t *dev;
    uint32_t start_tick;
    uint32_t last_tick;
    uint32_t period;
    uint32_t last_output_state;

} hbridge_instrument_t;

static inline volatile uint32_t get_current_tick()
{
    return *((volatile uint32_t *)0x20003004) / 10;
}

static inline volatile uint32_t get_current_usec()
{
    return *((volatile uint32_t *)0x20003004);
}

#define TICK_RATE 100000 // 100k usec clock

#endif