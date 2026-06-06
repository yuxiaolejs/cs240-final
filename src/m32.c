#include "osc.h"
#include "mem.h"

#define M32_PORT 10023
uint8_t dst_ip_src[4] = {10, 0, 5, 15};
uint8_t dst_ip[4] = {10, 0, 5, 15};

float fl_to_db(float value)
{
    if (value <= 0.00001)
        return 0;
    else if (value <= 0.0625)
        return -90 + value / 0.0625 * 30;
    else if (value <= 0.25)
        return -60 + (value - 0.0625) / (0.25 - 0.0625) * 30;
    else if (value <= 0.5)
        return -30 + (value - 0.25) / 0.25 * 20;
    else if (value <= 1.0)
        return -10 + (value - 0.5) / 0.5 * 20;
    else
        return 0;
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
        return 0;
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
    printk("Sending msg:");
    for (uint32_t i = 0; i < msg_len; i++)
    {
        printk("%x ", (uint8_t)msg2[i]);
    }
    printk("\n");
    memcpy(dst_ip, dst_ip_src, 4);
    printk("Dest IP: %d.%d.%d.%d\n", dst_ip[0], dst_ip[1], dst_ip[2], dst_ip[3]);
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
    printk("Sending msg:");
    for (uint32_t i = 0; i < msg_len; i++)
    {
        printk("%x ", (uint8_t)msg2[i]);
    }
    printk("\n");
    memcpy(dst_ip, dst_ip_src, 4);
    printk("Dest IP: %d.%d.%d.%d\n", dst_ip[0], dst_ip[1], dst_ip[2], dst_ip[3]);
    if (w5500_udp_sendto(0, dst_ip, M32_PORT, (const uint8_t *)msg2, msg_len) < 0)
    {
        printk("on off send failed for fader %d\n", fader_id);
        return;
    }
    kfree(msg1);
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
        delay_ms(100);
    }
    kfree(buf);
}

float m32_get_meter(uint8_t fader_id)
{
    char *msg1 = kalloc(100);
    char *msg2 = kalloc(100);
    float output = 0.0f;
    uint32_t msg_len = encode_message(msg2, "/meters", "s", "/meters/0");
    printk("Sending msg:");
    for (uint32_t i = 0; i < msg_len; i++)
    {
        printk("%x ", (uint8_t)msg2[i]);
    }
    printk("\n");
    memcpy(dst_ip, dst_ip_src, 4);
    if (w5500_udp_sendto(0, dst_ip, M32_PORT, (const uint8_t *)msg2, msg_len) < 0)
    {
        printk("meterUDP send failed for fader %d\n", fader_id);
        return output;
    }
    uint8_t rdata[600], rpath[100], rtypes[100];
    m32_recv(rpath, rtypes, rdata);
    m32_recv(rpath, rtypes, rdata);
    m32_recv(rpath, rtypes, rdata);
    m32_recv(rpath, rtypes, rdata);
    m32_recv(rpath, rtypes, rdata);
    m32_recv(rpath, rtypes, rdata);
    m32_recv(rpath, rtypes, rdata);
    m32_recv(rpath, rtypes, rdata);
    memcpy(&output, rdata + 4 * fader_id, sizeof(float));
    kfree(msg1);
    kfree(msg2);
    return output;
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

    delay_ms(100);
    float mapped = db_to_fl(0.0f);
    printk("Mapped 0 dB to %d\n", (int8_t)(mapped * 100));
    for (uint8_t i = 1; i < 9; i++)
    {
        m32_set_on(i, 1);
        m32_set_fader(i, mapped);
        m32_get_meter(i);
        m32_get_meter(i);
        m32_get_meter(i);
        printk("Got meter\n");
    }

    // m32_set_fader(1, 0.8250);
    // m32_set_fader(1, 0.8250);

    printk("UDP sent\n");
    uint8_t buf[2048];
    char *types = kalloc(100);
    uint32_t data[100];
    while (1)
    {
        int res = w5500_udp_recv_one(0, buf, 1024);
        if (res > 0)
        {
            uint32_t rec_len = (buf[6] << 8) | buf[7];
            printk("recv result: %d, srcip %d.%d.%d.%d, length %d\n", res, buf[0], buf[1], buf[2], buf[3], rec_len);
            printk("Raw message: '%s'\n", buf + 8);
            decode_message((char *)buf + 8, msg, types, (uint8_t *)data);
            printk("Decoded message path: %s\n", msg);
            printk("Decoded message types: %s\n", types);
            printk("Decoded message datas %d %d %d %d\n", data[0], data[1], data[2], data[3]);
            printk("Decoded message datas as strings:\n  '%s'\n  '%s'\n  '%s'\n  '%s'\n", buf + 8 + data[0], buf + 8 + data[1], buf + 8 + data[2], buf + 8 + data[3]);
            printk("Received UDP packet: '%s'\n", buf + 8);
        }
        delay_ms(100);
    }
}