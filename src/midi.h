#ifndef MIDI_H
#define MIDI_H
#include "types.h"
typedef struct midi_event
{
    uint32_t channel;
    uint32_t freq;
} midi_event;

// static inline float midi_to_hz(uint8_t n)
// {
//     return 440.0f * powf(2.0f, (n - 69) / 12.0f);
// }
midi_event midi_scan(uint8_t gpio);

#endif