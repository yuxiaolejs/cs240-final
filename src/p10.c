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
#include "dma.h"

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
        sleep_us(1);
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

#define TEXT_MOD_COLS 64
#define TEXT_MOD_ROWS 48
#include "font_4x8.h"
uint32_t text_x = 0;
uint32_t text_y = 0;
uint32_t p10_font_rows = font4x8[0];
uint32_t p10_font_cols = font4x8[1];

void put_pixel(uint32_t x, uint32_t y, uint8_t color)
{
    if (x >= ARRAY_WIDTH_P10 * P10_COLS_PER_ROW * 8 || y >= ARRAY_HEIGHT_P10 * P10_HEIGHT)
        return;
    uint32_t idx = y * ARRAY_WIDTH_P10 * P10_COLS_PER_ROW * 8 + x;
    recv_buffer[8 + 1 + idx / 8] |= (color << (7 - (idx % 8)));
}

void p10_handle_scroll()
{
    uint8_t *render_buf = recv_buffer + 8 + 1;
    if (text_y >= TEXT_MOD_ROWS)
    {
        memmove_fast((void *)render_buf, (void *)(render_buf + p10_font_rows * TEXT_MOD_COLS / 8), (TEXT_MOD_ROWS - p10_font_rows) * TEXT_MOD_COLS / 8);
        // clear bottom font_rows pixels
        // for (int i = 480 - font_rows; i < 480; i++)
        //     for (int j = 0; j < pitch / 4; j++)
        //         fb[i * (pitch / 4) + j] = 0;
        memset_fast((void *)(render_buf + (TEXT_MOD_ROWS - p10_font_rows) * TEXT_MOD_COLS / 8), 0, p10_font_rows * TEXT_MOD_COLS / 8);
        text_y -= p10_font_rows;
    }
}

void text_mode_render(char *str)
{
    uint8_t *render_buf = recv_buffer + 8 + 1;
    while (*str)
    {
        if ((*str < 0x20 || *str > 0x7E) && *str != '\n')
        {
            printk("Unsupported character '%x' in printToScreen\n", *str);
            str++;
            continue;
        }
        if (*str == '\n') // just do the \r\n thingy
        {
            text_y += p10_font_rows;
            text_x = 0;
            p10_handle_scroll();
        }
        else
        {
            for (uint32_t r = 0; r < p10_font_rows; r++)
                for (uint32_t c = 0; c < p10_font_cols; c++)
                {
                    uint8_t pixel_on = font4x8[2 + (*str - 0x20) * p10_font_rows + r] & (1 << (p10_font_cols - 1 - c));
                    put_pixel(text_x + c, text_y + r, pixel_on ? 1 : 0);
                }
            text_x += p10_font_cols;
        }
        if (text_x >= TEXT_MOD_COLS)
        {
            text_x = 0;
            text_y += p10_font_rows;
            p10_handle_scroll();
        }
        str++;
    }
    单接口单向一路带数据(recv_buffer + 8 + 1, frame_buffer);
}

void p10_start_render_loop()
{
    while (1)
    {
        SPI_RENDER(frame_buffer);
        rpi_yield();
    }
}
void auto_tune_sequence()
{
    m32_prob();
}
__attribute__((aligned(32))) dma_cb_t spi_dma_cb;
__attribute__((aligned(32))) dma_cb_t spi_dma_cb_rx;
__attribute__((aligned(256))) dma_cb_t p10_offset_table;
__attribute__((aligned(256))) dma_cb_t spi_dma_cb_rx_addr;
void p10_start_server()
{
    memset(frame_buffer, 0xff, sizeof(frame_buffer));
    if (w5500_udp_open(1, 9800) < 0)
    {
        printk("[p10] UDP socket open failed\n");
        text_mode_render("+p10+ UDP socket open failed\n");
        return;
    }
    gpio_set_output(OE);
    gpio_set_output(ADD_A);
    gpio_set_output(ADD_B);
    gpio_set_output(SCLK);

    spi_dma_cb.ti = DMA_TI_SPI_TX;
    spi_dma_cb.source_ad = 0;
    spi_dma_cb.dest_ad = 0x7E204004;
    spi_dma_cb.txfr_len = SPI_XFER_SIZE;
    spi_dma_cb.stride = 0;
    spi_dma_cb.nextconbk = 0;
    spi_dma_cb.reserved1 = 0;
    spi_dma_cb.reserved2 = 0;

    spi_dma_cb_rx.ti = DMA_TI_SPI_RX;
    spi_dma_cb_rx.source_ad = 0x7E204004;
    spi_dma_cb_rx.dest_ad = 0;
    spi_dma_cb_rx.txfr_len = SPI_XFER_SIZE;
    spi_dma_cb_rx.stride = 0;
    spi_dma_cb_rx.nextconbk = 0;
    spi_dma_cb_rx.reserved1 = 0;
    spi_dma_cb_rx.reserved2 = 0;

    // MK_REG(0x20007ff0) |= (1 << 1); // DMA_ENABLE: channel 1
    // MK_REG(DMA_CONBLK_AD_REG(1)) = ARM_TO_DMA_BUS(&spi_dma_cb_rx);
    *((uint32_t *)&spi_dma_cb_rx_addr) = ARM_TO_DMA_BUS(&spi_dma_cb_rx);
    *((uint32_t *)&p10_offset_table) = ARM_TO_DMA_BUS((uint8_t)&frame_buffer);

    uint32_t *p10_offset_table_u32 = (uint32_t *)&p10_offset_table;
    *p10_offset_table_u32 = ARM_TO_DMA_BUS((uint32_t)&frame_buffer);
    *(p10_offset_table_u32 + 1) = ARM_TO_DMA_BUS((uint32_t)&frame_buffer + 96);
    *(p10_offset_table_u32 + 2) = ARM_TO_DMA_BUS((uint32_t)&frame_buffer + 96 * 2);
    *(p10_offset_table_u32 + 3) = ARM_TO_DMA_BUS((uint32_t)&frame_buffer + 96 * 3);
    *(p10_offset_table_u32 + 4) = ARM_TO_DMA_BUS((uint32_t)&p10_offset_table);

    for (int i = 0; i < 4; i++)
    {
        printk("fb gt: %x at %x\n", p10_offset_table_u32[i], &p10_offset_table_u32[i]);
    }

    run_dma();
    printk("DMA RUNNING\n");
    printk("TFLEN %d\n", spi_dma_cb_rx.txfr_len);
    auto_tune_sequence();
    printk("[p10] UDP socket opened\n");
    text_mode_render("UDP ready@9800\n");
    char ip_str[16];
    w5500_get_ip(ip_str);
    text_mode_render(ip_str);
    w5500_get_subnet(ip_str);
    text_mode_render("\n");
    text_mode_render(ip_str);
    w5500_get_gateway(ip_str);
    text_mode_render("\n");
    text_mode_render(ip_str);
    while (1)
    {
        // printk("SPI STATUS %b, CH1_CS: %b, len %d, dma2_ctrl: %x, should be: %x, xfer len: %d\n", MK_REG(0x20204000) >> 16, MK_REG(0x20007100), MK_REG(0x2020400c), MK_REG(DMA_CONBLK_AD_REG(1)), spi_dma_cb_rx_addr, spi_dma_cb_rx.txfr_len);
        // printk("FB ADD: %x\n", spi_dma_cb.source_ad);
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