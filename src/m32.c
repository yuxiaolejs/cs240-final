#include "osc.h"
#include "mem.h"
#include <stdarg.h>
#include "midi.h"

#define DEBUG_OSC 0
#define DEFAULT_GAIN_DB 40.0f

#define M32_PORT 10023
uint8_t dst_ip_src[4] = {10, 0, 5, 15};
uint8_t dst_ip[4] = {10, 0, 5, 15};

float fl_to_db(float value)
{
    if (value <= 0.00001)
        return -9999;
    else if (value <= 0.0625)
        return -90 + value / 0.0625 * 30;
    else if (value <= 0.25)
        return -60 + (value - 0.0625) / (0.25 - 0.0625) * 30;
    else if (value <= 0.5)
        return -30 + (value - 0.25) / 0.25 * 20;
    else if (value <= 1.0)
        return -10 + (value - 0.5) / 0.5 * 20;
    else
        return 10;
}

float db_to_fl(float value)
{
    if (value <= -90)
        return 0;
    else if (value <= -60)
        return 0.0625 * (value + 90) / 30;
    else if (value <= -30)
        return (0.25 - 0.0625) * (value + 60) / 30 + 0.0625;
    else if (value <= -10)
        return 0.25 * (value + 30) / 20 + 0.25;
    else if (value <= 10)
        return 0.5 * (value + 10) / 20 + 0.5;
    else
        return 1.0f;
}

float gain_db_to_fl(float gain_db)
{
    float val = (gain_db + 12.0f) / 72.0f;
    if (val < 0.0f)
        val = 0.0f;
    else if (val > 1.0f)
        val = 1.0f;
    return val;
}

void m32_set_fader(uint8_t fader_id, float value)
{
    char *msg1 = kalloc(100);
    char *msg2 = kalloc(100);
    if (fader_id < 10)
        sprintf(msg1, "/ch/0%d/mix/fader", fader_id);
    else
        sprintf(msg1, "/ch/%d/mix/fader", fader_id);
    uint32_t msg_len = encode_message(msg2, msg1, "f", value);
    memcpy(dst_ip, dst_ip_src, 4);
    if (w5500_udp_sendto(0, dst_ip, M32_PORT, (const uint8_t *)msg2, msg_len) < 0)
    {
        printk("faderUDP send failed for fader %d\n", fader_id);
        return;
    }
    kfree(msg1);
    kfree(msg2);
}

void m32_set_on(uint8_t fader_id, uint8_t value)
{
    char *msg1 = kalloc(100);
    char *msg2 = kalloc(100);
    if (fader_id < 10)
        sprintf(msg1, "/ch/0%d/mix/on", fader_id);
    else
        sprintf(msg1, "/ch/%d/mix/on", fader_id);
    uint32_t msg_len = encode_message(msg2, msg1, "i", value);
    // printk("Sending msg:");
    // for (uint32_t i = 0; i < msg_len; i++)
    // {
    //     printk("%x ", (uint8_t)msg2[i]);
    // }
    // printk("\n");
    memcpy(dst_ip, dst_ip_src, 4);
    if (w5500_udp_sendto(0, dst_ip, M32_PORT, (const uint8_t *)msg2, msg_len) < 0)
    {
        printk("on off send failed for fader %d\n", fader_id);
        return;
    }
    kfree(msg1);
    kfree(msg2);
}

void m32_chann_ctrl(uint8_t chann, char *ctrl, char *type, ...)
{
    va_list args;
    va_start(args, type);
    char *msg1 = kalloc(100);
    char *msg2 = kalloc(100);
    if (chann < 10)
        sprintf(msg1, "/ch/0%d/%s", chann, ctrl);
    else
        sprintf(msg1, "/ch/%d/%s", chann, ctrl);
    if (DEBUG_OSC)
        printk("Control path: %s\n", msg1);
    uint32_t msg_len = encode_message_va(msg2, msg1, type, args);
    memcpy(dst_ip, dst_ip_src, 4);
    if (w5500_udp_sendto(0, dst_ip, M32_PORT, (const uint8_t *)msg2, msg_len) < 0)
    {
        printk("m32_chann_ctrl send failed for channel %d\n", chann);
        return;
    }
    sleep_ms(10);
    kfree(msg1);
    kfree(msg2);
    va_end(args);
}

void m32_headamp_ctrl(uint8_t chann, char *ctrl, char *type, ...)
{
    va_list args;
    va_start(args, type);
    char *msg1 = kalloc(100);
    char *msg2 = kalloc(100);
    if (chann < 10)
        sprintf(msg1, "/headamp/00%d/%s", chann, ctrl);
    else if (chann < 100)
        sprintf(msg1, "/headamp/0%d/%s", chann, ctrl);
    else
        sprintf(msg1, "/headamp/%d/%s", chann, ctrl);
    if (DEBUG_OSC)
        printk("Control path: %s\n", msg1);
    uint32_t msg_len = encode_message_va(msg2, msg1, type, args);
    memcpy(dst_ip, dst_ip_src, 4);
    if (w5500_udp_sendto(0, dst_ip, M32_PORT, (const uint8_t *)msg2, msg_len) < 0)
    {
        printk("m32_headamp_ctrl send failed for channel %d\n", chann);
        return;
    }
    sleep_ms(10);
    kfree(msg1);
    kfree(msg2);
    va_end(args);
}

void m32_channel_init(uint8_t chann)
{
    printk("[channel init %d] Initializing channel %d\n", chann, chann);
    m32_chann_ctrl(chann, "mix/fader", "f", 0.0f);
    m32_chann_ctrl(chann, "mix/on", "i", 0);
    m32_chann_ctrl(chann, "eq/on", "i", 0);
    m32_chann_ctrl(chann, "dyn/on", "i", 0);
    m32_chann_ctrl(chann, "gate/on", "i", 0);
    m32_chann_ctrl(chann, "delay/on", "i", 0);
    m32_chann_ctrl(chann, "preamp/trim", "f", 0.0f);
    m32_chann_ctrl(chann, "config/name", "s", "cs240lx");
    m32_chann_ctrl(chann, "config/color", "i", 2);
    m32_headamp_ctrl(chann - 1, "gain", "f", gain_db_to_fl(DEFAULT_GAIN_DB));
    m32_headamp_ctrl(chann - 1, "phantom", "");
    uint8_t rdata[600], rpath[100], rtypes[100];
    m32_recv(rpath, rtypes, rdata);
    if (!str_eq(rpath + 13, "phantom"))
    {
        printk("[channel init %d] Unexpected response during channel initialization, expected phantom control response\n");
    }
    if (rdata[0] != 1)
    {
        printk("[channel init %d] Phantom power not detected, got %d\n", chann, rdata[0]);
    }
}

void m32_unsub()
{
    char *msg2 = kalloc(100);
    uint32_t msg_len = encode_message(msg2, "/unsubscribe", "");
    // printk("Sending unsub msg:");
    // for (uint32_t i = 0; i < msg_len; i++)
    // {
    //     printk("%x ", (uint8_t)msg2[i]);
    // }
    // printk("\n");
    memcpy(dst_ip, dst_ip_src, 4);
    if (w5500_udp_sendto(0, dst_ip, M32_PORT, (const uint8_t *)msg2, msg_len) < 0)
    {
        printk("unsubscribe send failed\n");
        return;
    }
    kfree(msg2);
}

void m32_recv(uint8_t *path, uint8_t *types, uint8_t *data)
{
    char *buf = kalloc(2048);
    while (1)
    {
        int res = w5500_udp_recv_one(0, buf, 1024);
        if (res > 0)
        {
            uint32_t rec_len = (buf[6] << 8) | buf[7];
            // printk("recv result: %d, srcip %d.%d.%d.%d, length %d\n", res, buf[0], buf[1], buf[2], buf[3], rec_len);
            // printk("Raw message: '%s'\n", buf + 8);
            decode_message((char *)buf + 8, (char *)path, (char *)types, data);
            // printk("Decoded message path: %s\n", path);
            // printk("Decoded message types: %s\n", types);
            // printk("Decoded message datas %d %d %d %d\n", data[0], data[1], data[2], data[3]);
            // printk("Decoded message datas as strings:\n  '%s'\n  '%s'\n  '%s'\n  '%s'\n", buf + 8 + data[0], buf + 8 + data[1], buf + 8 + data[2], buf + 8 + data[3]);
            // printk("Received UDP packet: '%s'\n", buf + 8);
            break;
        }
        sleep_ms(100);
    }
    kfree(buf);
}

float m32_get_meter(uint8_t fader_id)
{
    char *msg1 = kalloc(100);
    char *msg2 = kalloc(100);
    float output = 0.0f;
    uint32_t msg_len = encode_message(msg2, "/meters", "siii", "/meters/0", 0, 0, 99);
    // printk("Sending msg get meter:");
    // for (uint32_t i = 0; i < msg_len; i++)
    // {
    //     printk("%x ", (uint8_t)msg2[i]);
    // }
    // printk("\n");
    memcpy(dst_ip, dst_ip_src, 4);
    if (w5500_udp_sendto(0, dst_ip, M32_PORT, (const uint8_t *)msg2, msg_len) < 0)
    {
        printk("meterUDP send failed for fader %d\n", fader_id);
        return output;
    }
    sleep_ms(100);
    m32_unsub();
    uint8_t rdata[600], rpath[100], rtypes[100];
    m32_recv(rpath, rtypes, rdata);
    memcpy(&output, rdata + 4 * fader_id, sizeof(float));
    kfree(msg1);
    kfree(msg2);
    return output;
}

uint8_t channels_occupied[32] = {0};

void m32_midi_channel_tune(uint8_t midi_chann, float target_db)
{
    printk("[midi tune %d] Tuning channel %d\n", midi_chann, midi_chann);
    midi_event ev;
    ev.channel = midi_chann;
    ev.freq = 140;
    midi_apply(ev);
    // find m32 channel corresponding to midi
    // if (midi_chann == 3)
    // {
    //     while (1)
    //         sleep_ms(1000);
    // }
    int chann = -1;
    float max_meter = -1000.0f;
    sleep_ms(500);
    for (int i = 1; i <= 16; i++)
    {
        if (channels_occupied[i])
            continue;
        // printk("[midi tune %d] Checking M32 channel %d\n", midi_chann, i);
        float meter = fl_to_db(m32_get_meter(i));
        printk("[midi tune %d] M32 channel %d meter: %d mB\n", midi_chann, i, (int)(meter * 100));
        if (meter > max_meter && meter > -70.0f) // require at least -70 dB to consider it a match
        {
            max_meter = meter;
            chann = i;
        }
    }
    printk("[midi tune %d] MIDI channel %d mapped to M32 channel %d with meter %d mB\n", midi_chann, midi_chann, chann, (int)(max_meter * 100));
    if (chann == -1)
    {
        printk("[midi tune %d] No channel found for MIDI channel %d\n", midi_chann, midi_chann);
        ev.freq = 0;
        midi_apply(ev);
        return;
    }
    float db_diff = target_db - max_meter;
    if (db_diff > 10)
    {
        float gain_comp = db_diff - 10;
        if (gain_comp > 60)
            gain_comp = 60;
        m32_headamp_ctrl(chann - 1, "gain", "f", gain_db_to_fl(DEFAULT_GAIN_DB + gain_comp));
        db_diff = 10;
    }
    if (db_diff > 10 || db_diff < -90)
    {
        printk("[midi tune %d] Unreasonable target mB %d for MIDI channel %d with current meter %d, diff is %d\n", midi_chann, (int)(target_db * 100), midi_chann, (int)(max_meter * 100), (int)(db_diff * 100));
        ev.freq = 0;
        midi_apply(ev);
        return;
    }
    else
        printk("[midi tune %d] Adjusting M32 channel %d by %d mB to reach target %d mB\n", midi_chann, chann, (int)(db_diff * 100), (int)(target_db * 100));
    channels_occupied[chann] = 1 << 7 | midi_chann; // mark channel as occupied by this MIDI channel
    m32_chann_ctrl(chann, "mix/fader", "f", db_to_fl(db_diff));
    m32_chann_ctrl(chann, "mix/on", "i", 1);
    char chann_name[20];
    sprintf(chann_name, "MIDI %d", midi_chann);
    m32_chann_ctrl(chann, "config/name", "s", chann_name);
    m32_chann_ctrl(chann, "config/color", "i", 1);
    ev.freq = 0;
    midi_apply(ev);
}

void m32_prob()
{
    char *msg = kalloc(100);
    uint32_t msg_len = encode_message(msg, "/info", "");
    if (w5500_udp_open(0, M32_PORT) < 0)
        return;

    uint8_t rpath[100], rtypes[100], rdata[100];

    printk("UDP socket opened\n");
    printk("Dest IP: %d.%d.%d.%d\n", dst_ip[0], dst_ip[1], dst_ip[2], dst_ip[3]);
    if (w5500_udp_sendto(0, dst_ip, M32_PORT, (const uint8_t *)msg, msg_len) < 0)
    {
        printk("UDP send failed\n");
        return;
    }
    kfree(msg);

    m32_recv(rpath, rtypes, rdata);
    printk("recv path: %s, types: %s, data: %d %d %d %d\n", rpath, rtypes, rdata[0], rdata[1], rdata[2], rdata[3]);

    sleep_ms(100);
    float mapped = db_to_fl(-99);
    printk("Mapped 0 mB to %d\n", (int8_t)(mapped * 100));
    for (uint8_t i = 1; i < 9; i++)
    {
        m32_channel_init(i);
    }
    // for (uint8_t i = 1; i < 9; i++)
    // {
    //     m32_chann_ctrl(i, "mix/on", "i", 1);
    // }

    // m32_set_fader(1, 0.8250);
    // m32_set_fader(1, 0.8250);

    printk("UDP sent\n");
    memset(channels_occupied, 0, sizeof(channels_occupied));
    for (uint8_t i = 0; i < 6; i++)
    {
        m32_midi_channel_tune(i, -40.0f);
    }
    // uint8_t buf[2048];
    // char *types = kalloc(100);
    // uint32_t data[100];
    // while (1)
    // {
    //     int res = w5500_udp_recv_one(0, buf, 1024);
    //     if (res > 0)
    //     {
    //         uint32_t rec_len = (buf[6] << 8) | buf[7];
    //         printk("recv result: %d, srcip %d.%d.%d.%d, length %d\n", res, buf[0], buf[1], buf[2], buf[3], rec_len);
    //         printk("Raw message: '%s'\n", buf + 8);
    //         decode_message((char *)buf + 8, msg, types, (uint8_t *)data);
    //         printk("Decoded message path: %s\n", msg);
    //         printk("Decoded message types: %s\n", types);
    //         printk("Decoded message datas %d %d %d %d\n", data[0], data[1], data[2], data[3]);
    //         printk("Decoded message datas as strings:\n  '%s'\n  '%s'\n  '%s'\n  '%s'\n", buf + 8 + data[0], buf + 8 + data[1], buf + 8 + data[2], buf + 8 + data[3]);
    //         printk("Received UDP packet: '%s'\n", buf + 8);
    //     }
    //     sleep_ms(100);
    // }
}