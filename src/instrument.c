#include "instrument.h"

void open_drain_set(int gpio, int value)
{
    if (value)
    {
        gpio_set_input(gpio);
    }
    else
    {
        gpio_set_output(gpio);
        gpio_set_off(gpio);
    }
}

void push_pull_set(int gpio, int value)
{
    if (value)
    {
        gpio_set_on(gpio);
        gpio_set_output(gpio);
    }
    else
    {
        gpio_set_off(gpio);
        gpio_set_output(gpio);
    }
}

void tick_instrument(instrument_t *inst)
{
    uint32_t tick = get_current_tick();
    if (inst->type == INSTRUMENT_TYPE_FLOPPY)
    {
        floppy_instrument_t *floppy_inst = (floppy_instrument_t *)inst;
        if (floppy_inst->period == 0)
            return; // inst stopped
        // state[0] = step (primary) GPIO state, state[1] = dir (secondary) GPIO state, state[2] = counter for direction change
        if (tick / (inst->period / 2) != inst->last_tick / (inst->period / 2)) // half period for one toggle
        {
            if (floppy_inst->dir_counter++ >= 50)
            {
                floppy_inst->dir_counter = 0;
                floppy_inst->last_dir_state = !floppy_inst->last_dir_state;
                open_drain_set(floppy_inst->dev->gpio_secondary, floppy_inst->last_dir_state);
            }
            // Toggle step pin
            floppy_inst->last_step_state = !floppy_inst->last_step_state;
            open_drain_set(floppy_inst->dev->gpio_primary, floppy_inst->last_step_state);
            floppy_inst->last_tick = tick;
        }
        // here we don't do anything with this instrument.
    }
    else if (inst->type == INSTRUMENT_TYPE_HBRIDGE)
    {
        hbridge_instrument_t *hbridge_inst = (hbridge_instrument_t *)inst;
        if (hbridge_inst->period == 0)
        {
            push_pull_set(hbridge_inst->dev->gpio_primary, 0);
            push_pull_set(hbridge_inst->dev->gpio_secondary, 0);
            return;
        } // inst stopped
        if (tick / (inst->period / 2) != inst->last_tick / (inst->period / 2)) // half period for one toggle
        {
            hbridge_inst->last_output_state = !hbridge_inst->last_output_state;
            push_pull_set(hbridge_inst->dev->gpio_primary, hbridge_inst->last_output_state);
            push_pull_set(hbridge_inst->dev->gpio_secondary, !hbridge_inst->last_output_state);
            hbridge_inst->last_tick = tick;
        }
    }
    else if (inst->type == INSTRUMENT_TYPE_SPEAKER)
    {
    }
}