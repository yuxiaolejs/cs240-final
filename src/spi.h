#ifndef SPI_H
#include <stdarg.h>
#include <stdint.h>

#define SPI_MISO 9
#define SPI_SCK 11
#define SPI_CE 6
#define SPI_IRQ 22
#define SPI_MOSI 10
#define SPI_CS_1 8
#define SPI_CS_2 7

#define SPI_MMIO 0x20204000

#define SPI_CS_ADDR (SPI_MMIO + 0x00)
#define SPI_FIFO_ADDR (SPI_MMIO + 0x04)
#define SPI_CLK_ADDR (SPI_MMIO + 0x08)
#define SPI_DLEN_ADDR (SPI_MMIO + 0x0c)
#define SPI_LTOH_ADDR (SPI_MMIO + 0x10)
#define SPI_DC_ADDR (SPI_MMIO + 0x14)

#define SPI_CS_REG ((volatile uint32_t *)SPI_CS_ADDR)
#define SPI_FIFO_REG ((volatile uint32_t *)SPI_FIFO_ADDR)
#define SPI_CLK_REG ((volatile uint32_t *)SPI_CLK_ADDR)
#define SPI_DLEN_REG ((volatile uint32_t *)SPI_DLEN_ADDR)
#define SPI_LTOH_REG ((volatile uint32_t *)SPI_LTOH_ADDR)
#define SPI_DC_REG ((volatile uint32_t *)SPI_DC_ADDR)

#define SPI_CS_CLEAR_RXTX ((1 << 4) | (1 << 5))
#define SPI_CS_TA (1 << 7)
#define SPI_CS_DMAEN (1 << 8)
#define SPI_CS_DONE (1 << 16)

int spi_ok_to_send();
int spi_ok_to_recv();
int spi_xfer_done();
int spi_get_read_enable();
void spi_set_read_enable(int enable);
int spi_get_transfer_active();
void spi_start_transfer();
void spi_send_byte(uint8_t byte);
uint8_t spi_recv_byte();
void spi_flush_fifo();
void spi_sendn(uint8_t *buf, uint32_t len);
void spi_recvn(uint8_t *buf, uint32_t len);
void spi_xfern(uint8_t *buf, uint32_t len);
void spi_txn(uint8_t *buf, uint32_t len);
void spi_init(void);
#endif