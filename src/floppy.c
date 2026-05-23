#include "floppy.h"
#include "instrument.h"
#include "pictl.h"
#include "regs.h"
#include "gpio.h"
#include "timing.h"
#define POSITION_DELAY_CYCLES 7000 // 1us

void init_floppy(floppy_dev *dev)
{
    open_drain_set(dev->step_pin, 1);
    open_drain_set(dev->dir_pin, 1);
    gpio_set_input(dev->read_pin);
    gpio_set_pullup(dev->read_pin);
    gpio_set_input(dev->index_pin);
    gpio_set_pullup(dev->index_pin);
    open_drain_set(dev->side_pin, 1);
    dev->track_number = 0;
    for (int i = 0; i < 100; i++)
    {
        open_drain_set(dev->step_pin, 0);
        delay_ms(5);
        open_drain_set(dev->step_pin, 1);
        delay_ms(5);
    }
    *PTR_GPFEN0 |= (1 << dev->read_pin);
    // *PTR_GPREN0 |= (1 << dev->read_pin);
    *PTR_GPFEN0 |= (1 << dev->index_pin);
}

void move_to_track(floppy_dev *dev, uint8_t track)
{
    if (dev->track_number < track)
    {
        open_drain_set(dev->dir_pin, 0);
    }
    else if (dev->track_number > track)
    {
        open_drain_set(dev->dir_pin, 1);
    }
    while (dev->track_number != track)
    {
        open_drain_set(dev->step_pin, 0);
        delay_ms(5);
        open_drain_set(dev->step_pin, 1);
        delay_ms(5);
        if (dev->track_number < track)
            dev->track_number++;
        else
            dev->track_number--;
    }
}

static inline uint32_t next_position(uint32_t cycle)
{
    if (cycle_cnt_read() - cycle > POSITION_DELAY_CYCLES)
    {
        printk("[full] We fucked up: running behind by %d cycles\n", cycle_cnt_read() - cycle);
        rpi_reboot();
    }
    while (cycle_cnt_read() - cycle < POSITION_DELAY_CYCLES)
        ;
    return cycle + POSITION_DELAY_CYCLES;
}
static inline uint32_t next_half_position(uint32_t cycle)
{
    if (cycle_cnt_read() - cycle > POSITION_DELAY_CYCLES / 2)
    {
        printk("[half] We fucked up: running behind by %d cycles\n", cycle_cnt_read() - cycle);
        rpi_reboot();
    }
    while (cycle_cnt_read() - cycle < POSITION_DELAY_CYCLES / 2)
        ;
    return cycle + POSITION_DELAY_CYCLES / 2;
}

#define HISTOGRAM_SIZE 100

void floppy_read_track(floppy_dev *dev, uint8_t *buffer, uint32_t length)
{
    // This is a placeholder implementation. In a real implementation, you would read the data from the floppy disk using the read pin and store it in the buffer.
    printk("Start reading from floppy at track %d\n", dev->track_number);
    memset(buffer, 0, length);
    while (*PTR_GPLEV0 & (1 << dev->index_pin))
    {
    }
    uint32_t start_cycle = cycle_cnt_read();       // start the clock
    start_cycle = next_half_position(start_cycle); // align to the first position after index
    while (*PTR_GPLEV0 & (1 << dev->index_pin))
    {
        while (*PTR_GPLEV0 & (1 << dev->read_pin))
        {
        }
        uint32_t read_cycle = cycle_cnt_read();
        uint32_t pos = (read_cycle - start_cycle) / POSITION_DELAY_CYCLES;
        printk("High bit at cycle %d, pos %d\n", read_cycle, pos);
        while (!(*PTR_GPLEV0 & (1 << dev->read_pin)))
        {
        }
    }
}

void test_timing()
{
    uint32_t start = cycle_cnt_read();
    start = next_position(start);
    start = next_position(start);
    start = next_position(start);
    start = next_position(start);
    start = next_position(start);
    start = next_position(start);
    start = next_position(start);
    start = next_position(start);
    start = next_position(start);
    start = next_position(start);
    return;
}

void floppy_write_track(floppy_dev *dev, uint8_t *buffer, uint32_t length)
{
    test_timing();
    printk("Timing test completed, starting to write to floppy\n");
    // This is a placeholder implementation. In a real implementation, you would read the data from the floppy disk using the read pin and store it in the buffer.
    printk("Start writing to floppy at track %d\n", dev->track_number);
    while (*PTR_GPLEV0 & (1 << dev->index_pin))
    {
    }
    *PTR_GPFEN0 |= (1 << dev->index_pin); // enable index pin detect for write timing
    *PTR_GPEDS0 = (1 << dev->index_pin);
    uint32_t start_cycle = cycle_cnt_read(); // start the clock
    open_drain_set(dev->write_gate_pin, 0);  // enable write gate
    open_drain_set(dev->write_pin, 1);
    start_cycle = next_half_position(start_cycle); // align to the first position after index

    // start_cycle = next_half_position(start_cycle); // align to the first position after index
    uint32_t write_bit_idx = 0;
    while (!(*PTR_GPEDS0 & (1 << dev->index_pin)))
    {
        if ((buffer[write_bit_idx / 8] >> (7 - (write_bit_idx % 8))) & 1)
        {
            printk("+");
            open_drain_set(dev->write_pin, 0);
            start_cycle = next_half_position(start_cycle); // align to the first position after index
            open_drain_set(dev->write_pin, 1);
        }
        else
        {
            printk(".");
        }
        write_bit_idx++;
        if (write_bit_idx >= length * 8)
            break;
        start_cycle = next_position(start_cycle);
    }
    open_drain_set(dev->write_gate_pin, 1);
    printk("Finished writing to floppy, total bits written: %d\n", write_bit_idx);
}