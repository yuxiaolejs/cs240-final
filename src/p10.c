// ref https://github.com/raspberrypi/pico-examples/blob/master/i2c/mpu6050_i2c/mpu6050_i2c.c
#include "cstr.h"
#include "serial.h"
#include "mem.h"
#include "pictl.h"
#include "gpio.h"
#include "regs.h"
#include "p10.h"
#include "w5500.h"
#include "instrument.h"
#include "midi.h"

void short_delay()
{
    sleep_us(1);
}

void gpio_set(int pin, int value)
{
    if (value)
    {
        // gpio_set_input(pin);
        gpio_set_on(pin);
    }
    else
    {
        gpio_set_off(pin);
        // gpio_set_output(pin);
    }
}

void pulse_clk()
{
    gpio_set(CLK, 1);
    short_delay();
    gpio_set(CLK, 0);
    short_delay();
}

void pulse_lat()
{
    gpio_set(SCLK, 1);
    short_delay();
    gpio_set(SCLK, 0);
    short_delay();
}

void set_row_addr(int row_group)
{
    gpio_set(ADD_A, row_group & 1);
    gpio_set(ADD_B, (row_group >> 1) & 1);
}

void SPI_RENDER(uint8_t *frame)
{
    for (int group = 0; group < 4; group++)
    {
        set_row_addr(group);
        asm volatile("mcr p15, 0, %0, c7, c10, 5"
                     :
                     : "r"(0)
                     : "memory");
        spi_txn_dma(frame + group * (ARRAY_FRAME_SIZE / 4), ARRAY_FRAME_SIZE / 4);
        asm volatile("mcr p15, 0, %0, c7, c10, 5"
                     :
                     : "r"(0)
                     : "memory");
        pulse_lat();
        asm volatile("mcr p15, 0, %0, c7, c10, 5"
                     :
                     : "r"(0)
                     : "memory");
        gpio_set(OE, 1);
        sleep_us(10);
        gpio_set(OE, 0);
    }
}

void 单接口单向一路带数据(uint8_t *framein, uint8_t *frameout)
{
    uint32_t out_idx = 0;
    for (int group = 0; group < 4; group++)
    {
        for (int board_y = ARRAY_HEIGHT_P10 - 1; board_y >= 0; board_y--)
        {
            int dir = 1;
            for (int board_x = (dir == 1) ? 0 : (ARRAY_WIDTH_P10 - 1); board_x >= 0 && board_x < ARRAY_WIDTH_P10; board_x += dir)
            {
                for (int x = 0; x < P10_COLS_PER_ROW; x++)
                {
                    for (int y = 0; y < P10_ROWS_PER_GROUP; y++)
                    {
                        uint32_t pixel_x = board_x * P10_COLS_PER_ROW + x;
                        uint32_t pixel_y = board_y * P10_HEIGHT + (3 - y) * P10_ROWS_PER_GROUP + group;
                        uint32_t idx = pixel_y * ARRAY_WIDTH_P10 * P10_COLS_PER_ROW + pixel_x;
                        // printk("board_x: %d, board_y: %d, x: %d, y: %d, idx: %d\n", board_x, board_y, x, y, idx);
                        frameout[out_idx++] = framein[idx] ^ 0xFF;
                    }
                }
            }
            rpi_yield();
        }
    }
}

uint32_t frame_time;
uint8_t frame_buffer[ARRAY_FRAME_SIZE];
uint8_t recv_buffer[ARRAY_FRAME_SIZE + 8 + 6];
#define FRAME_TIME_US 33366
void p10_start_render_loop()
{
    while (1)
    {
        SPI_RENDER(frame_buffer);
        rpi_yield();
    }
}
void p10_start_server()
{
    if (w5500_udp_open(1, 9800) < 0)
    {
        printk("[p10] UDP socket open failed\n");
        return;
    }
    gpio_set_output(OE);
    gpio_set_output(ADD_A);
    gpio_set_output(ADD_B);
    gpio_set_output(SCLK);
    printk("[p10] UDP socket opened\n");
    memset(frame_buffer, 0xff, sizeof(frame_buffer));
    while (1)
    {
        rpi_yield();
        int recv = w5500_udp_recv_one(1, recv_buffer, ARRAY_FRAME_SIZE + 8 + 6 + 1);
        if (recv == 0)
            continue;
        // printk("UDP recv time: %d ticks, render time: %d ticks\n", get_current_tick() - udp_time, udp_time - start);
        // printk("ctrl: %d\n", recv_buffer[8]);
        if (recv_buffer[8] == 0x93)
            单接口单向一路带数据(recv_buffer + 8 + 1, frame_buffer);
        else if (recv_buffer[8] == 0x94)
        {
            midi_event ps = midi_parse(recv_buffer[8 + 1], recv_buffer[8 + 2]);
            midi_apply(ps);
        }
    }
}

// void notmain()
// {
//     printk("online\n");
//     gpio_set_output(OE);
//     gpio_set_output(ADD_A);
//     gpio_set_output(ADD_B);
//     gpio_set_output(CLK);
//     gpio_set_output(SCLK);
//     gpio_set_output(DATA);
//     main();
// }