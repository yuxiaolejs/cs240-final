#ifndef FLOPPY_H
#define FLOPPY_H
#include "types.h"

typedef struct floppy_dev
{
    uint8_t step_pin;
    uint8_t dir_pin;
    uint8_t read_pin;
    uint8_t side_pin;
    uint8_t index_pin;
    uint8_t write_gate_pin;
    uint8_t write_pin;
    uint8_t track_number;
    uint32_t last_index_cycle;
} floppy_dev;

void init_floppy(floppy_dev *dev);
void floppy_read_track(floppy_dev *dev, uint8_t *buffer, uint32_t length);
#endif