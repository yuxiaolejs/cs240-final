#include "spi.h"
#include "dma.h"

int spi_ok_to_send()
{
    uint32_t status = *SPI_CS_REG;
    return (status & (1 << 18)) != 0;
}
int spi_ok_to_recv()
{
    uint32_t status = *SPI_CS_REG;
    return (status & (1 << 17)) != 0;
}
int spi_xfer_done()
{
    uint32_t status = *SPI_CS_REG;
    return (status & (1 << 16)) != 0;
}
int spi_get_read_enable()
{
    uint32_t status = *SPI_CS_REG;
    return (status & (1 << 12)) != 0;
}
void spi_set_read_enable(int enable)
{
    if (enable)
        *SPI_CS_REG |= (1 << 12);
    else
        *SPI_CS_REG &= ~(1 << 12);
}

int spi_get_transfer_active()
{
    uint32_t status = *SPI_CS_REG;
    return (status & (1 << 7)) != 0;
}

void spi_start_transfer()
{
    *SPI_CS_REG |= (1 << 7);
}

void spi_send_byte(uint8_t byte)
{
    while (!spi_ok_to_send())
        ;
    *SPI_FIFO_REG = byte;
}
uint8_t spi_recv_byte()
{
    while (!spi_ok_to_recv())
        ;
    return *SPI_FIFO_REG & 0xFF;
}

void spi_flush_fifo()
{
    *SPI_CS_REG |= (0b11 << 4);
}

void spi_sendn(uint8_t *buf, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
        spi_send_byte(buf[i]);
}
void spi_recvn(uint8_t *buf, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
        buf[i] = spi_recv_byte();
}

void spi_xfern(uint8_t *buf, uint32_t len)
{
    *SPI_CS_REG |= (3 << 4); // clear TX RX
    *SPI_CS_REG |= (1 << 7);
    for (uint32_t i = 0; i < len; i++)
    {
        spi_send_byte(buf[i]);
        buf[i] = spi_recv_byte();
    }
    while (!(*SPI_CS_REG & (1 << 16)))
        ;
    *SPI_CS_REG &= ~(1 << 7);
}

void spi_txn(uint8_t *buf, uint32_t len)
{
    *SPI_CS_REG |= (3 << 4); // clear TX RX
    *SPI_CS_REG |= (1 << 7);
    for (uint32_t i = 0; i < len; i++)
    {
        spi_send_byte(buf[i]);
        spi_recv_byte();
    }
    while (!(*SPI_CS_REG & (1 << 16)))
        ;
    *SPI_CS_REG &= ~(1 << 7);
}

__attribute__((aligned(32))) static dma_cb_t spi_dma_cb;
__attribute__((aligned(32))) static dma_cb_t spi_dma_cb_rx;

void spi_txn_dma(uint8_t *buf, uint32_t len)
{
    *SPI_CS_REG |= (3 << 4); // clear TX RX
    *SPI_DLEN_REG = len;
    spi_dma_cb.ti = DMA_TI_SPI_TX;
    spi_dma_cb.source_ad = ARM_TO_DMA_BUS(buf);
    spi_dma_cb.dest_ad = 0x7E204004;
    spi_dma_cb.txfr_len = len;
    spi_dma_cb.stride = 0;
    spi_dma_cb.nextconbk = 0;
    spi_dma_cb.reserved1 = 0;
    spi_dma_cb.reserved2 = 0;

    spi_dma_cb_rx.ti = DMA_TI_SPI_RX;
    spi_dma_cb_rx.source_ad = 0x7E204004;
    spi_dma_cb_rx.dest_ad = 0;
    spi_dma_cb_rx.txfr_len = len;
    spi_dma_cb_rx.stride = 0;
    spi_dma_cb_rx.nextconbk = 0;
    spi_dma_cb_rx.reserved1 = 0;
    spi_dma_cb_rx.reserved2 = 0;

    *SPI_CS_REG |= SPI_CS_DMAEN | SPI_CS_TA;
    MK_REG(DMA_CONBLK_AD_REG(0)) = ARM_TO_DMA_BUS(&spi_dma_cb);
    MK_REG(DMA_CS_REG(0)) = DMA_CS_ACTIVE;
    MK_REG(DMA_CONBLK_AD_REG(1)) = ARM_TO_DMA_BUS(&spi_dma_cb_rx);
    MK_REG(DMA_CS_REG(1)) = DMA_CS_ACTIVE;
    while (!(MK_REG(DMA_CS_REG(0)) & DMA_CS_END))
        ;
    while (!(*SPI_CS_REG & (1 << 16)))
        ;
    *SPI_CS_REG &= ~(1 << 7);
}

void spi_init(void)
{
    gpio_pin_set_func(SPI_MISO, 0b100); // set to alt0
    gpio_pin_set_func(SPI_MOSI, 0b100);
    gpio_pin_set_func(SPI_SCK, 0b100);
    // gpio_pin_set_func(SPI_CS_1, 0b100);
    // gpio_pin_set_func(SPI_CS_2, 0b100);
    // gpio_pin_set_func(5, 1); // set to output
    // gpio_pin_set_func(6, 1); // set to output
    // gpio_set_off(5);
    // gpio_set_off(6);
    *SPI_CLK_REG = 32; // ~3.9 MHz
    *SPI_CS_REG = 0;   // CS0, mode 0,0
}