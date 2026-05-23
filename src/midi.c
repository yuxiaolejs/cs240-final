#include "midi.h"
#define SPEED 700000000
#define baud_rate 31250
#define bit_time_cycles (SPEED / baud_rate)
#include "instrument.h"

static float logf_(float x)
{
    // Range reduce: x = 2^n * m, m in [1,2)
    int n = 0;
    while (x >= 2.0f)
    {
        x *= 0.5f;
        n++;
    }
    while (x < 1.0f)
    {
        x *= 2.0f;
        n--;
    }
    // ln(m) via ln(m) = 2*(y + y^3/3 + y^5/5 + ...), y = (m-1)/(m+1)
    float y = (x - 1.0f) / (x + 1.0f);
    float y2 = y * y, t = y, s = y;
    for (int i = 3; i < 20; i += 2)
    {
        t *= y2;
        s += t / i;
    }
    return 2.0f * s + (float)n * 0.6931472f; // + n*ln(2)
}

static float expf_(float x)
{
    // Range reduce: x = k*ln2 + r, r in ~[-ln2/2, ln2/2]
    const float LN2 = 0.6931472f;
    int k = (int)(x / LN2 + (x >= 0 ? 0.5f : -0.5f));
    float r = x - (float)k * LN2;
    // e^r via Taylor
    float s = 1.0f, t = 1.0f;
    for (int i = 1; i < 15; i++)
    {
        t *= r / (float)i;
        s += t;
    }
    // multiply by 2^k
    while (k > 0)
    {
        s *= 2.0f;
        k--;
    }
    while (k < 0)
    {
        s *= 0.5f;
        k++;
    }
    return s;
}

float powf(float base, float exp)
{
    if (base <= 0.0f)
        return 0.0f; // your call how to handle; self-test won't hit this
    return expf_(exp * logf_(base));
}

float midi_to_hz_lookup_table[128];
void midi_init()
{
    for (int i = 0; i < 128; i++)
    {
        midi_to_hz_lookup_table[i] = 440.0f * powf(2.0f, (i - 69) / 12.0f);
    }
}

static inline float midi_to_hz(uint8_t n)
{
    // return 440.0f * powf(2.0f, (n - 69) / 12.0f);
    return midi_to_hz_lookup_table[n];
}

static inline void cycle_cnt_init(void)
{
    uint32_t in = 1;
    asm volatile("MCR p15, 0, %0, c15, c12, 0" ::"r"(in));
}

// read cycle counter: should add a write().
static inline uint32_t cycle_cnt_read(void)
{
    uint32_t out;
    asm volatile("MRC p15, 0, %0, c15, c12, 1" : "=r"(out));
    return out;
}

void delay_cycles_(uint32_t cycles)
{
    cycle_cnt_init();
    uint32_t now = cycle_cnt_read();
    while (cycle_cnt_read() - now < cycles)
        ;
}

static inline uint8_t midi_recv(uint8_t gpio)
{
    uint32_t start_time = cycle_cnt_read();
    while (gpio_read(gpio) == 1)
    {
        if (cycle_cnt_read() - start_time > bit_time_cycles * 10)
            return 0; // timeout, no data
    }
    delay_cycles_(bit_time_cycles / 2);
    uint8_t data = 0;
    for (int i = 0; i < 8; i++)
    {
        delay_cycles_(bit_time_cycles);
        data |= (gpio_read(gpio) << i);
    }
    delay_cycles_(bit_time_cycles);
    return data;
}

midi_event midi_scan(uint8_t gpio)
{
    uint8_t b = midi_recv(gpio);
    if (b == 0 || b == 0xFF)
        return (midi_event){-1, 0}; // ignore invalid data
    uint8_t note = midi_recv(gpio);
    midi_recv(gpio); // ignore velocity
    if (b >= 0xF8)
        return (midi_event){-1, 0}; // real-time, ignore
    midi_event res;
    if (b & 0x80)
    { // status byte
        if (b >= 0x80 && b <= 0x9F)
        { // Note Off / Note On
            if (b < 0x90)
            {
                res.channel = b & 0x0F;
                res.freq = 0;
                return res;
            }
            else
            {
                res.channel = b & 0x0F;
                res.freq = midi_to_hz(note);
                return res;
            }
        }
    }
    return (midi_event){-1, 0};
}

midi_event midi_parse(uint8_t b, uint8_t note)
{
    if (b == 0 || b == 0xFF)
        return (midi_event){-1, 0}; // ignore invalid data
    if (b >= 0xF8)
        return (midi_event){-1, 0}; // real-time, ignore
    midi_event res;
    if (b & 0x80)
    { // status byte
        if (b >= 0x80 && b <= 0x9F)
        { // Note Off / Note On
            if (b < 0x90)
            {
                res.channel = b & 0x0F;
                res.freq = 0;
                return res;
            }
            else
            {
                res.channel = b & 0x0F;
                res.freq = midi_to_hz(note);
                return res;
            }
        }
    }
    return (midi_event){-1, 0};
}