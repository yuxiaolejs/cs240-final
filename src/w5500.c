#include "w5500.h"
#include "mem.h"
#include "instrument.h"

static void _5500_parse_ip(char *ip_str, uint8_t *ip_bytes)
{
    uint8_t *ptr = ip_bytes;
    uint8_t num = 0;
    while (*ip_str)
    {
        if (*ip_str >= '0' && *ip_str <= '9')
        {
            num = num * 10 + (*ip_str - '0');
        }
        else if (*ip_str == '.')
        {
            *ptr++ = num;
            num = 0;
        }
        else
        {
            printk("Invalid IP string: %s\n", ip_str);
            return;
        }
        ip_str++;
    }
    *ptr = num;
}

int w5500_xfer(uint16_t addr, uint8_t block, uint8_t is_write, uint8_t *data, uint16_t len)
{
    uint8_t *xfer_buffer = kalloc(len + 3);
    xfer_buffer[0] = (addr >> 8) & 0xFF;
    xfer_buffer[1] = addr & 0xFF;
    xfer_buffer[2] = (block << 3) | (is_write ? W5500_WRITE : W5500_READ) | W5500_VDM;
    if (is_write)
    {
        memcpy(xfer_buffer + 3, data, len);
        minispi_xfern(xfer_buffer, len + 3);
    }
    else
    {
        minispi_xfern(xfer_buffer, len + 3);
        memcpy(data, xfer_buffer + 3, len);
    }
    kfree(xfer_buffer);
    return 0; // TODO: error handling for
}

int w5500_xfer_zero_cpy(uint16_t addr, uint8_t block, uint8_t is_write, uint8_t *xfer_buffer, uint16_t len)
{
    xfer_buffer[0] = (addr >> 8) & 0xFF;
    xfer_buffer[1] = addr & 0xFF;
    xfer_buffer[2] = (block << 3) | (is_write ? W5500_WRITE : W5500_READ) | W5500_VDM;
    if (is_write)
        minispi_xfern(xfer_buffer, len + 3);
    else
        minispi_xfern(xfer_buffer, len + 3);
    return 0; // TODO: error handling for
}

int w5500_probe(void)
{
    uint8_t version = 0;

    w5500_xfer(W5500_REG_VERSIONR,
               BSB_COMMON,
               0, // read
               &version,
               1);

    printk("W5500 VERSIONR = 0x%x\n", version);

    if (version != 0x04)
    {
        printk("W5500 not detected\n");
        return -1;
    }

    printk("W5500 detected\n");
    return 0;
}

void w5500_check_physical()
{
    uint8_t phycfgr = 0;
    w5500_xfer(W5500_REG_PHYCFGR, BSB_COMMON, 0, &phycfgr, 1);
    printk("W5500 PHYCFGR = 0x%x\n", phycfgr);
    printk("  Link: %s\n", (phycfgr & 0x01) ? "up" : "down");
    printk("  Speed: %s\n", (phycfgr & 0x02) ? "100Mbps" : "10Mbps");
    printk("  Duplex: %s\n", (phycfgr & 0x04) ? "full" : "half");
}

void w5500_get_ip(char *ip_str)
{
    uint8_t ip2[4] = {0};
    w5500_xfer(W5500_REG_SIPR, BSB_COMMON, 0, ip2, 4);
    sprintf(ip_str, "%d.%d.%d.%d", ip2[0], ip2[1], ip2[2], ip2[3]);
}

void w5500_set_ip(char *ip_str)
{
    uint8_t ip[4] = {0};
    _5500_parse_ip(ip_str, ip);
    w5500_xfer(W5500_REG_SIPR, BSB_COMMON, 1, ip, 4);
}

void w5500_get_subnet(char *ip_str)
{
    uint8_t ip2[4] = {0};
    w5500_xfer(W5500_REG_SUBR, BSB_COMMON, 0, ip2, 4);
    sprintf(ip_str, "%d.%d.%d.%d", ip2[0], ip2[1], ip2[2], ip2[3]);
}

void w5500_set_subnet(char *ip_str)
{
    uint8_t ip[4] = {0};
    _5500_parse_ip(ip_str, ip);
    w5500_xfer(W5500_REG_SUBR, BSB_COMMON, 1, ip, 4);
}

void w5500_get_gateway(char *ip_str)
{
    uint8_t ip2[4] = {0};
    w5500_xfer(W5500_REG_GAR, BSB_COMMON, 0, ip2, 4);
    sprintf(ip_str, "%d.%d.%d.%d", ip2[0], ip2[1], ip2[2], ip2[3]);
}

void w5500_set_gateway(char *ip_str)
{
    uint8_t ip[4] = {0};
    _5500_parse_ip(ip_str, ip);
    w5500_xfer(W5500_REG_GAR, BSB_COMMON, 1, ip, 4);
}

uint8_t w5500_read8(uint16_t addr, uint8_t block)
{
    uint8_t v = 0;
    w5500_xfer(addr, block, 0, &v, 1);
    return v;
}

void w5500_write8(uint16_t addr, uint8_t block, uint8_t v)
{
    w5500_xfer(addr, block, 1, &v, 1);
}

uint16_t w5500_read16(uint16_t addr, uint8_t block)
{
    uint8_t b[5];
    w5500_xfer_zero_cpy(addr, block, 0, b, 2);
    return ((uint16_t)b[3] << 8) | b[4];
}

void w5500_write16(uint16_t addr, uint8_t block, uint16_t v)
{
    uint8_t b[2];

    b[0] = v >> 8;
    b[1] = v & 0xff;

    w5500_xfer(addr, block, 1, b, 2);
}

void w5500_read_buf(uint16_t addr, uint8_t block, uint8_t *buf, uint16_t len)
{
    w5500_xfer(addr, block, 0, buf, len);
}

void w5500_write_buf(uint16_t addr, uint8_t block, const uint8_t *buf, uint16_t len)
{
    w5500_xfer(addr, block, 1, (uint8_t *)buf, len);
}

int w5500_sock_cmd(uint8_t sock, uint8_t cmd)
{
    uint8_t reg = BSB_SOCK_REG(sock);
    w5500_write8(Sn_CR, reg, cmd);
    while (w5500_read8(Sn_CR, reg) != 0)
        ;
    return 0;
}

int w5500_udp_open(uint8_t sock, uint16_t local_port)
{
    uint8_t reg = BSB_SOCK_REG(sock);

    w5500_sock_cmd(sock, CR_CLOSE);

    w5500_write8(Sn_MR, reg, Sn_MR_UDP);
    w5500_write16(Sn_PORT, reg, local_port);

    w5500_sock_cmd(sock, CR_OPEN);

    uint8_t sr = w5500_read8(Sn_SR, reg);
    if (sr != SOCK_UDP)
    {
        printk("UDP open failed: Sn_SR=0x%x\n", sr);
        return -1;
    }

    return 0;
}

int w5500_write_tx_buffer_safe(uint8_t sock, uint16_t offset, const uint8_t *data, uint16_t len)
{
    uint8_t tx_block = BSB_SOCK_TX(sock);
    if (offset + len > W5500_SOCK_BUF_SIZE)
    {
        uint16_t first = W5500_SOCK_BUF_SIZE - offset;
        w5500_write_buf(offset, tx_block, data, first);
        w5500_write_buf(0, tx_block, data + first, len - first);
    }
    else
        w5500_write_buf(offset, tx_block, data, len);
    return 0;
}

int w5500_udp_sendto(uint8_t sock, const uint8_t dst_ip[4], uint16_t dst_port, const uint8_t *data, uint16_t len)
{
    uint8_t reg = BSB_SOCK_REG(sock);

    if (w5500_read8(Sn_SR, reg) != SOCK_UDP)
    {
        printk("socket is not UDP: Sn_SR=0x%x\n", w5500_read8(Sn_SR, reg));
        return -1;
    }

    w5500_write_buf(Sn_DIPR, reg, dst_ip, 4);
    w5500_write16(Sn_DPORT, reg, dst_port);

    while (w5500_read16(Sn_TX_FSR, reg) < len)
        ;

    uint16_t wr = w5500_read16(Sn_TX_WR, reg);

    w5500_write_tx_buffer_safe(sock, wr, data, len);

    wr += len;
    w5500_write16(Sn_TX_WR, reg, wr);
    w5500_sock_cmd(sock, CR_SEND);

    while (1)
    {
        uint8_t ir = w5500_read8(Sn_IR, reg);
        if (ir & Sn_IR_SENDOK)
        {
            w5500_write8(Sn_IR, reg, Sn_IR_SENDOK);
            return 0;
        }
        if (ir & Sn_IR_TIMEOUT)
        {
            w5500_write8(Sn_IR, reg, Sn_IR_TIMEOUT);
            return -1;
        }
    }
    return 0;
}

int w5500_read_recv_buffer_safe(uint8_t sock, uint16_t offset, uint8_t *buf, uint16_t len)
{
    uint8_t rx_block = BSB_SOCK_RX(sock);
    offset = offset % W5500_SOCK_BUF_SIZE;
    if (offset + len > W5500_SOCK_BUF_SIZE)
    {
        uint16_t first = (W5500_SOCK_BUF_SIZE - offset);
        w5500_read_buf(offset, rx_block, buf, first);
        w5500_read_buf(0, rx_block, buf + first, len - first);
    }
    else
        w5500_read_buf(offset, rx_block, buf, len);
    return len;
}

int w5500_udp_recv_one(uint8_t sock, uint8_t *buf, uint16_t maxlen)
{
    uint8_t reg = BSB_SOCK_REG(sock);

    uint16_t rx_size = w5500_read16(Sn_RX_RSR, reg);
    if (rx_size == 0)
        return 0;

    uint16_t rd = w5500_read16(Sn_RX_RD, reg);
    uint16_t len = 0;
    w5500_read_recv_buffer_safe(sock, rd, buf, 8);
    len = ((uint16_t)buf[6] << 8) | buf[7];
    rd += 8;
    if (len > maxlen - 8)
    {
        printk("Received packet too large for buffer: %d bytes\n", len);
        return -1;
    }
    w5500_read_recv_buffer_safe(sock, rd, buf + 8, len);
    rd += len;
    w5500_write16(Sn_RX_RD, reg, rd);
    w5500_sock_cmd(sock, CR_RECV);

    return len + 8;
}

void w5500_init()
{
    w5500_probe();
    w5500_check_physical();
    char ip_str[16];
    w5500_set_ip(IP_ADDR);
    w5500_set_subnet(SUBNET);
    w5500_set_gateway(GATEWAY);
    w5500_get_ip(ip_str);
    printk("IP Address: %s\n", ip_str);
    w5500_get_subnet(ip_str);
    printk("Subnet Mask: %s\n", ip_str);
    w5500_get_gateway(ip_str);
    printk("Gateway: %s\n", ip_str);
}