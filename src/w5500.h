#ifndef W5500_H
#define W5500_H

#include "spi.h"
#include <stdint.h>

#define IP_ADDR "192.168.10.183"
#define SUBNET "255.255.255.0"
#define GATEWAY "192.168.10.1"

#define W5500_SOCK_BUF_SIZE 2048

#define W5500_READ 0x00
#define W5500_WRITE 0x04
#define W5500_VDM 0x00

#define BSB_COMMON 0x00

#define BSB_SOCK_REG(n) (0x01 + (n) * 4)
#define BSB_SOCK_TX(n) (0x02 + (n) * 4)
#define BSB_SOCK_RX(n) (0x03 + (n) * 4)

#define W5500_REG_MR 0x0000       // chip mode register / soft reset
#define W5500_REG_GAR 0x0001      // gateway IP, 4 bytes
#define W5500_REG_SUBR 0x0005     // subnet mask, 4 bytes
#define W5500_REG_SHAR 0x0009     // source MAC address, 6 bytes
#define W5500_REG_SIPR 0x000F     // source IP address, 4 bytes
#define W5500_REG_PHYCFGR 0x002E  // PHY config/status
#define W5500_REG_VERSIONR 0x0039 // chip version, should be 0x04
#define Sn_MR 0x0000              // Socket Mode Register.
#define Sn_CR 0x0001              // Socket Command Register.
#define Sn_IR 0x0002              // Socket Interrupt Register.
#define Sn_SR 0x0003              // Socket Status Register.
#define Sn_PORT 0x0004            // Socket Source Port Register, 16-bit.
#define Sn_DIPR 0x000C            // Socket Destination IP Address Register, 4 bytes.
#define Sn_DPORT 0x0010           // Socket Destination Port Register, 16-bit.
#define Sn_TX_FSR 0x0020          // Socket TX Free Size Register, 16-bit.
#define Sn_TX_WR 0x0024           // Socket TX Write Pointer Register, 16-bit.
#define Sn_MR_UDP 0x02            // UDP mode.
#define Sn_RX_RSR 0x0026          // Socket RX Received Size Register, 16-bit.
#define Sn_RX_RD 0x0028           // Socket RX Read Pointer Register, 16-bit.
#define Sn_IR_SENDOK 0x10         // Send completed successfully.
#define Sn_IR_TIMEOUT 0x08        // Timeout happened, often ARP/connect/send timeout.
#define Sn_IR_RECV 0x04           // Data received.
#define Sn_IR_DISCON 0x02         // TCP disconnect event.
#define Sn_IR_CON 0x01            // TCP connection established event.
#define CR_OPEN 0x01              // Open socket.
#define CR_CLOSE 0x10             // Close socket immediately.
#define CR_SEND 0x20              // Send data currently staged in the socket TX buffer.
#define CR_RECV 0x40              // Notify received data has been consumed.
#define SOCK_CLOSED 0x00          // Socket is closed / unused.
#define SOCK_UDP 0x22             // Socket is open in UDP mode.

int w5500_xfer(uint16_t addr, uint8_t block, uint8_t is_write, uint8_t *data, uint16_t len);
int w5500_probe(void);
void w5500_check_physical();

void w5500_get_ip(char *ip_str);
void w5500_set_ip(char *ip_str);

void w5500_get_subnet(char *ip_str);
void w5500_set_subnet(char *ip_str);

void w5500_get_gateway(char *ip_str);
void w5500_set_gateway(char *ip_str);

uint8_t w5500_read8(uint16_t addr, uint8_t block);
void w5500_write8(uint16_t addr, uint8_t block, uint8_t v);
uint16_t w5500_read16(uint16_t addr, uint8_t block);
void w5500_write16(uint16_t addr, uint8_t block, uint16_t v);
void w5500_write_buf(uint16_t addr, uint8_t block, const uint8_t *buf, uint16_t len);
int w5500_sock_cmd(uint8_t sock, uint8_t cmd);
int w5500_udp_open(uint8_t sock, uint16_t local_port);
int w5500_udp_sendto(uint8_t sock, const uint8_t dst_ip[4], uint16_t dst_port, const uint8_t *data, uint16_t len);
int w5500_udp_recv_one(uint8_t sock, uint8_t *buf, uint16_t maxlen);
#endif