#ifndef P10_H
#define P10_H

#include "types.h"

#define OE 25
#define ADD_A 22
#define ADD_B 23
#define CLK 11
#define SCLK 24
#define DATA 10

#define P10_WIDTH 32
#define P10_HEIGHT 16
#define P10_ROWS_PER_GROUP (P10_HEIGHT / 4)
#define P10_COLS_PER_ROW (P10_WIDTH / 8)
#define P10_FRAME_SIZE (P10_WIDTH * P10_HEIGHT / 8)
#define P10_GROUP_SIZE (P10_ROWS_PER_GROUP * P10_COLS_PER_ROW)

#define ARRAY_WIDTH_P10 2
#define ARRAY_HEIGHT_P10 3
#define ARRAY_FRAME_SIZE (ARRAY_WIDTH_P10 * ARRAY_HEIGHT_P10 * P10_FRAME_SIZE)

#define SPI_XFER_SIZE (ARRAY_FRAME_SIZE / 4)

void 单接口单向一路带数据(uint8_t *framein, uint8_t *frameout);
void SPI_RENDER(uint8_t *frame);
void p10_start_server();
void p10_start_render_loop();

#endif