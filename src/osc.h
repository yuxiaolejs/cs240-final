#ifndef OSC_H
#define OSC_H

#include <stdint.h>
uint32_t encode_message(char *buf, char *path, char *types, ...);
uint32_t decode_message(char *buf, char *path, char *types, uint8_t *data);

#endif