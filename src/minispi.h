#ifndef MINISPI_H
#define MINISPI_H
#include <stdarg.h>
#include <stdint.h>

// AUX SPI 0 (a.k.a. SPI1, "Mini SPI") on the BCM2835.
// BCM2835 ARM Peripherals datasheet, ch.2 Auxiliaries.
//
// GPIO pin assignments (all on ALT4):
//   GPIO18 = SPI1_CE0_N
//   GPIO17 = SPI1_CE1_N
//   GPIO16 = SPI1_CE2_N
//   GPIO19 = SPI1_MISO
//   GPIO20 = SPI1_MOSI
//   GPIO21 = SPI1_SCLK
#define MINISPI_CE0 18
#define MINISPI_CE1 17
#define MINISPI_CE2 16
#define MINISPI_MISO 19
#define MINISPI_MOSI 20
#define MINISPI_SCK 21

// Auxiliary peripheral register block (ARM physical = BCM 0x7E215000)
#define AUX_BASE 0x20215000

#define AUX_IRQ_ADDR (AUX_BASE + 0x00)
#define AUX_ENABLES_ADDR (AUX_BASE + 0x04)
#define AUX_SPI0_CNTL0_ADDR (AUX_BASE + 0x80)
#define AUX_SPI0_CNTL1_ADDR (AUX_BASE + 0x84)
#define AUX_SPI0_STAT_ADDR (AUX_BASE + 0x88)
#define AUX_SPI0_PEEK_ADDR (AUX_BASE + 0x8C)
// IO writes deassert CS at end of access; TXHOLD writes keep CS asserted.
#define AUX_SPI0_IO_ADDR (AUX_BASE + 0xA0)
#define AUX_SPI0_TXHOLD_ADDR (AUX_BASE + 0xB0)

#define AUX_IRQ_REG ((volatile uint32_t *)AUX_IRQ_ADDR)
#define AUX_ENABLES_REG ((volatile uint32_t *)AUX_ENABLES_ADDR)
#define MINISPI_CNTL0_REG ((volatile uint32_t *)AUX_SPI0_CNTL0_ADDR)
#define MINISPI_CNTL1_REG ((volatile uint32_t *)AUX_SPI0_CNTL1_ADDR)
#define MINISPI_STAT_REG ((volatile uint32_t *)AUX_SPI0_STAT_ADDR)
#define MINISPI_PEEK_REG ((volatile uint32_t *)AUX_SPI0_PEEK_ADDR)
#define MINISPI_IO_REG ((volatile uint32_t *)AUX_SPI0_IO_ADDR)
#define MINISPI_TXHOLD_REG ((volatile uint32_t *)AUX_SPI0_TXHOLD_ADDR)

// AUX_ENABLES bits
#define AUX_ENABLES_SPI1 (1 << 1)

// CNTL0 bit positions
#define MINISPI_CNTL0_SPEED_SHIFT 20
#define MINISPI_CNTL0_CS_SHIFT 17
#define MINISPI_CNTL0_VAR_CS (1 << 15)
#define MINISPI_CNTL0_VAR_WIDTH (1 << 14)
#define MINISPI_CNTL0_ENABLE (1 << 11)
#define MINISPI_CNTL0_IN_RISING (1 << 10)
#define MINISPI_CNTL0_CLR_FIFOS (1 << 9)
#define MINISPI_CNTL0_OUT_RISING (1 << 8)
#define MINISPI_CNTL0_INVERT_CLK (1 << 7)
#define MINISPI_CNTL0_MSB_FIRST (1 << 6)

// CNTL1 bit positions
#define MINISPI_CNTL1_MSB_FIRST_IN (1 << 1)

// STAT bits. NOTE: the BCM2835 datasheet's AUX_SPI_STAT table lists these at
// bits 4/3/2 — that's a documentation error. The real hardware places them
// alongside Busy in the [10:6] range, as Mike McCauley's bcm2835 library and
// Linux's spi-bcm2835aux both use.
#define MINISPI_STAT_TX_FULL (1 << 10)
#define MINISPI_STAT_TX_EMPTY (1 << 9)
#define MINISPI_STAT_RX_FULL (1 << 8)
#define MINISPI_STAT_RX_EMPTY (1 << 7)
#define MINISPI_STAT_BUSY (1 << 6)

int minispi_ok_to_send();
int minispi_ok_to_recv();
int minispi_xfer_done();
void minispi_send_byte(uint8_t byte);
uint8_t minispi_recv_byte();
void minispi_flush_fifo();
void minispi_sendn(uint8_t *buf, uint32_t len);
void minispi_recvn(uint8_t *buf, uint32_t len);
void minispi_xfern(uint8_t *buf, uint32_t len);
void minispi_init(void);
#endif
