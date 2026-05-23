#include "minispi.h"
#include "gpio.h"
#include "regs.h"
#include "mem.h"

// AUX SPI 0 driver, variable-width mode. Configured for SPI mode 0
// (CPOL=0, CPHA=0), MS-first, CE0 active.
//
// Variable-width mode (CNTL0 bit 14 = 1) takes the shift length from the
// transmit FIFO entry itself (bits [28:24]) instead of from CNTL0[5:0].
//
// Data positions for an 8-bit shift, MS-first:
//   TX byte → bits [23:16] of the FIFO entry (left-justified to bit 23,
//             which is what the datasheet means by "first bit shifted out
//             will be bit 23").
//   RX byte ← bits [7:0] of the FIFO entry. The shift register shifts
//             left during the transfer, so MISO bits accumulate at the
//             LSB end as the TX bits exit at the MSB end.
//
// One byte per FIFO entry: write (8 << 24) | (byte << 16); read & 0xFF.

int minispi_ok_to_send()
{
    return (*MINISPI_STAT_REG & MINISPI_STAT_TX_FULL) == 0;
}

int minispi_ok_to_recv()
{
    return (*MINISPI_STAT_REG & MINISPI_STAT_RX_EMPTY) == 0;
}

int minispi_xfer_done()
{
    return (*MINISPI_STAT_REG & MINISPI_STAT_BUSY) == 0;
}

void minispi_send_byte(uint8_t byte)
{
    while (!minispi_ok_to_send())
        ;
    *MINISPI_IO_REG = (8u << 24) | ((uint32_t)byte << 16);
}

uint8_t minispi_recv_byte()
{
    while (!minispi_ok_to_recv())
        ;
    // Received byte lands in [7:0]: the shift register shifts left during
    // the transfer, so MISO bits accumulate at the LSB end.
    return *MINISPI_IO_REG & 0xFF;
}

void minispi_flush_fifo()
{
    *MINISPI_CNTL0_REG |= MINISPI_CNTL0_CLR_FIFOS;
    *MINISPI_CNTL0_REG &= ~MINISPI_CNTL0_CLR_FIFOS;
}

// Interleave TX submissions with RX drains so the 4-deep FIFOs don't deadlock.
// Use TXHOLD for all writes except the final one (which uses IO) so CS stays
// asserted across the whole burst.
void minispi_xfern(uint8_t *buf, uint32_t len)
{
    uint32_t tx_i = 0, rx_i = 0;
    while (rx_i < len)
    {
        if (rx_i % 5 == 0)
            rpi_yield();
        if (tx_i < len && minispi_ok_to_send())
        {
            uint32_t word = (8u << 24) | ((uint32_t)buf[tx_i] << 16);
            if (tx_i == len - 1)
                *MINISPI_IO_REG = word;
            else
                *MINISPI_TXHOLD_REG = word;
            tx_i++;
        }
        if (minispi_ok_to_recv())
        {
            buf[rx_i] = *MINISPI_IO_REG & 0xFF;
            rx_i++;
        }
    }
}

void minispi_xfern_no_end(uint8_t *buf, uint32_t len)
{
    uint32_t tx_i = 0, rx_i = 0;
    while (rx_i < len)
    {
        if (rx_i % 5 == 0)
            rpi_yield();
        if (tx_i < len && minispi_ok_to_send())
        {
            uint32_t word = (8u << 24) | ((uint32_t)buf[tx_i] << 16);
            *MINISPI_TXHOLD_REG = word;
            tx_i++;
        }
        if (minispi_ok_to_recv())
        {
            buf[rx_i] = *MINISPI_IO_REG & 0xFF;
            rx_i++;
        }
    }
}

void minispi_sendn(uint8_t *buf, uint32_t len)
{
    uint32_t tx_i = 0, rx_drained = 0;
    while (tx_i < len)
    {
        if (minispi_ok_to_send())
        {
            uint32_t word = (8u << 24) | ((uint32_t)buf[tx_i] << 16);
            if (tx_i == len - 1)
                *MINISPI_IO_REG = word;
            else
                *MINISPI_TXHOLD_REG = word;
            tx_i++;
        }
        if (minispi_ok_to_recv())
        {
            (void)*MINISPI_IO_REG;
            rx_drained++;
        }
    }
    while (rx_drained < len)
    {
        if (minispi_ok_to_recv())
        {
            (void)*MINISPI_IO_REG;
            rx_drained++;
        }
    }
}

void minispi_recvn(uint8_t *buf, uint32_t len)
{
    uint32_t tx_i = 0, rx_i = 0;
    while (rx_i < len)
    {
        if (tx_i < len && minispi_ok_to_send())
        {
            uint32_t word = (8u << 24);
            if (tx_i == len - 1)
                *MINISPI_IO_REG = word;
            else
                *MINISPI_TXHOLD_REG = word;
            tx_i++;
        }
        if (minispi_ok_to_recv())
        {
            buf[rx_i] = *MINISPI_IO_REG & 0xFF;
            rx_i++;
        }
    }
}

void minispi_init(void)
{
    // 1) Pin muxing — SPI1 functions are all on ALT4.
    gpio_pin_set_func(MINISPI_MISO, GPIO_ALT4);
    gpio_pin_set_func(MINISPI_MOSI, GPIO_ALT4);
    gpio_pin_set_func(MINISPI_SCK, GPIO_ALT4);
    gpio_pin_set_func(MINISPI_CE0, GPIO_ALT4);

    // Cross-peripheral barrier: we just finished writes to the GPIO block
    // and are about to write to the AUX block. Per BCM2835 datasheet p.7
    // these can be reordered, which silently breaks AUX_ENABLES.
    dmb();

    // 2) Enable the SPI1 module in the AUX block. Without this, register
    //    accesses to the SPI1 block are silently ignored.
    *AUX_ENABLES_REG |= AUX_ENABLES_SPI1;
    dmb();

    // 3) Configure CNTL0:
    //    - Speed = 31 → spi_clk = 250 MHz / (2*(31+1)) ≈ 3.9 MHz
    //    - CS pattern 0b110 → CE0 asserted (active low), CE1/CE2 inactive
    //      (bit 17 = CE0, bit 18 = CE1, bit 19 = CE2)
    //    - VAR_WIDTH = 1 → shift length comes from FIFO entry [28:24]
    //    - Enable = 1
    //    - In rising = 1, Out rising = 0, Invert CLK = 0 → SPI mode 0
    //    - MS bit first
    const uint32_t speed = 10;
    const uint32_t cs_pattern = 0b110;
    *MINISPI_CNTL0_REG =
        (speed << MINISPI_CNTL0_SPEED_SHIFT) |
        (cs_pattern << MINISPI_CNTL0_CS_SHIFT) |
        MINISPI_CNTL0_VAR_WIDTH |
        MINISPI_CNTL0_ENABLE |
        MINISPI_CNTL0_IN_RISING |
        MINISPI_CNTL0_MSB_FIRST;

    // 4) Configure CNTL1: shift-in MS bit first, no IRQs, no keep-input.
    *MINISPI_CNTL1_REG = MINISPI_CNTL1_MSB_FIRST_IN;

    // 5) Flush any stale FIFO state.
    minispi_flush_fifo();

    dmb();
}
