
#include "dma.h"
static uint8_t lut_add[65536] __attribute__((aligned(65536)));
static uint8_t lut_mult[65536] __attribute__((aligned(65536)));
static uint8_t lut_and[65536] __attribute__((aligned(65536)));
static uint8_t lut_orr[65536] __attribute__((aligned(65536)));
static uint8_t lut_xor[65536] __attribute__((aligned(65536)));
static uint8_t lut_rsh[65536] __attribute__((aligned(65536)));
static uint8_t lut_lsh[65536] __attribute__((aligned(65536)));
static uint8_t lut_imm[256] __attribute__((aligned(256)));
static int8_t lut_sub[65536] __attribute__((aligned(65536)));
static int8_t lut_tez[256] __attribute__((aligned(256)));
static int8_t lut_tnz[256] __attribute__((aligned(256)));
extern dma_cb_t spi_dma_cb __attribute__((aligned(32)));
extern dma_cb_t p10_offset_table __attribute__((aligned(32)));
extern dma_cb_t spi_dma_cb_rx_addr __attribute__((aligned(32)));

static dma_cb_t cb[75] __attribute__((aligned(32)));

static uint32_t lda_1100[2] __attribute__((aligned(256)));
static uint32_t lda_1c50[2] __attribute__((aligned(256)));
static uint32_t lda_2c50[2] __attribute__((aligned(256)));
static uint32_t lda_3c50[2] __attribute__((aligned(256)));
uint8_t e, row;

uint32_t word_temp;


void run_dma()
{


    for (int i = 0; i < 65536; i++)
        lut_add[i] = (uint8_t)((i >> 8) + (i & 0xff));
    for (int i = 0; i < 65536; i++)
        lut_mult[i] = (uint8_t)((i >> 8) * (i & 0xff));
    for (int i = 0; i < 65536; i++)
        lut_and[i] = (uint8_t)((i >> 8) & (i & 0xff));
    for (int i = 0; i < 65536; i++)
        lut_orr[i] = (uint8_t)((i >> 8) | (i & 0xff));
    for (int i = 0; i < 65536; i++)
        lut_xor[i] = (uint8_t)((i >> 8) ^ (i & 0xff));
    for (int i = 0; i < 65536; i++)
        lut_rsh[i] = (uint8_t)((i >> 8) >> (i & 31));
    for (int i = 0; i < 65536; i++)
        lut_lsh[i] = (uint8_t)((i >> 8) << (i & 31));
    for (int i = 0; i < 65536; i++)
        lut_sub[i] = (int8_t)((i >> 8) - (i & 0xff));
    for (int i = 0; i < 256; i++)
    {
        lut_imm[i] = (uint8_t)i;
        lut_tez[i] = (int8_t)(i == 0 ? 4 : 0);
        lut_tnz[i] = (int8_t)(i != 0 ? 4 : 0);
    }


    // Initialize branch labels
    lda_1100[0] = ARM_TO_DMA_BUS(&cb[33]);
    lda_1100[1] = ARM_TO_DMA_BUS(&cb[28]);
    lda_1c50[0] = ARM_TO_DMA_BUS(&cb[49]);
    lda_1c50[1] = ARM_TO_DMA_BUS(&cb[43]);
    lda_2c50[0] = ARM_TO_DMA_BUS(&cb[57]);
    lda_2c50[1] = ARM_TO_DMA_BUS(&cb[51]);
    lda_3c50[0] = ARM_TO_DMA_BUS(&cb[66]);
    lda_3c50[1] = ARM_TO_DMA_BUS(&cb[60]);

    // 0000: mov word_temp.0 #0
    cb[0].ti = DMA_TI_WAIT_RESP;
    cb[0].source_ad = ARM_TO_DMA_BUS(lut_imm) + 0;
    cb[0].dest_ad = ARM_TO_DMA_BUS(&word_temp) + 0;
    cb[0].txfr_len = 1;
    cb[0].nextconbk = ARM_TO_DMA_BUS(&cb[1]);

    // 0001: mov word_temp.1 #0
    cb[1].ti = DMA_TI_WAIT_RESP;
    cb[1].source_ad = ARM_TO_DMA_BUS(lut_imm) + 0;
    cb[1].dest_ad = ARM_TO_DMA_BUS(&word_temp) + 1;
    cb[1].txfr_len = 1;
    cb[1].nextconbk = ARM_TO_DMA_BUS(&cb[2]);

    // 0002: mov word_temp.2 #0
    cb[2].ti = DMA_TI_WAIT_RESP;
    cb[2].source_ad = ARM_TO_DMA_BUS(lut_imm) + 0;
    cb[2].dest_ad = ARM_TO_DMA_BUS(&word_temp) + 2;
    cb[2].txfr_len = 1;
    cb[2].nextconbk = ARM_TO_DMA_BUS(&cb[3]);

    // 0003: mov word_temp.3 #0
    cb[3].ti = DMA_TI_WAIT_RESP;
    cb[3].source_ad = ARM_TO_DMA_BUS(lut_imm) + 0;
    cb[3].dest_ad = ARM_TO_DMA_BUS(&word_temp) + 3;
    cb[3].txfr_len = 1;
    cb[3].nextconbk = ARM_TO_DMA_BUS(&cb[4]);

    // 0004: mov 0010.1 row
    cb[4].ti = DMA_TI_WAIT_RESP;
    cb[4].source_ad = ARM_TO_DMA_BUS(&row) + 0;
    cb[4].dest_ad = ARM_TO_DMA_BUS(&cb[5].source_ad) + 1;
    cb[4].txfr_len = 1;
    cb[4].nextconbk = ARM_TO_DMA_BUS(&cb[5]);

    // 0010: lsh word_temp.2 #6
    cb[5].ti = DMA_TI_WAIT_RESP;
    cb[5].source_ad = ARM_TO_DMA_BUS(lut_lsh) + 6;
    cb[5].dest_ad = ARM_TO_DMA_BUS(&word_temp) + 2;
    cb[5].txfr_len = 1;
    cb[5].nextconbk = ARM_TO_DMA_BUS(&cb[6]);

    // 0020: movw $0x7e20001C word_temp
    cb[6].ti = DMA_TI_WAIT_RESP;
    cb[6].source_ad = ARM_TO_DMA_BUS(&word_temp) + 0;
    cb[6].dest_ad = 0x7e20001C + 0;
    cb[6].txfr_len = 4;
    cb[6].nextconbk = ARM_TO_DMA_BUS(&cb[7]);

    // 0030: mov 0040.1 word_temp.2
    cb[7].ti = DMA_TI_WAIT_RESP;
    cb[7].source_ad = ARM_TO_DMA_BUS(&word_temp) + 2;
    cb[7].dest_ad = ARM_TO_DMA_BUS(&cb[8].source_ad) + 1;
    cb[7].txfr_len = 1;
    cb[7].nextconbk = ARM_TO_DMA_BUS(&cb[8]);

    // 0040: xor word_temp.2 #0b11000000
    cb[8].ti = DMA_TI_WAIT_RESP;
    cb[8].source_ad = ARM_TO_DMA_BUS(lut_xor) + 0b11000000;
    cb[8].dest_ad = ARM_TO_DMA_BUS(&word_temp) + 2;
    cb[8].txfr_len = 1;
    cb[8].nextconbk = ARM_TO_DMA_BUS(&cb[9]);

    // 0050: movw $0x7e200028 word_temp
    cb[9].ti = DMA_TI_WAIT_RESP;
    cb[9].source_ad = ARM_TO_DMA_BUS(&word_temp) + 0;
    cb[9].dest_ad = 0x7e200028 + 0;
    cb[9].txfr_len = 4;
    cb[9].nextconbk = ARM_TO_DMA_BUS(&cb[10]);

    // 1000: movw word_temp $0x7e204000
    cb[10].ti = DMA_TI_WAIT_RESP;
    cb[10].source_ad = 0x7e204000 + 0;
    cb[10].dest_ad = ARM_TO_DMA_BUS(&word_temp) + 0;
    cb[10].txfr_len = 4;
    cb[10].nextconbk = ARM_TO_DMA_BUS(&cb[11]);

    // 1001: mov 1010.1 word_temp
    cb[11].ti = DMA_TI_WAIT_RESP;
    cb[11].source_ad = ARM_TO_DMA_BUS(&word_temp) + 0;
    cb[11].dest_ad = ARM_TO_DMA_BUS(&cb[12].source_ad) + 1;
    cb[11].txfr_len = 1;
    cb[11].nextconbk = ARM_TO_DMA_BUS(&cb[12]);

    // 1010: orr word_temp #0b110000
    cb[12].ti = DMA_TI_WAIT_RESP;
    cb[12].source_ad = ARM_TO_DMA_BUS(lut_orr) + 0b110000;
    cb[12].dest_ad = ARM_TO_DMA_BUS(&word_temp) + 0;
    cb[12].txfr_len = 1;
    cb[12].nextconbk = ARM_TO_DMA_BUS(&cb[13]);

    // 1020: movw $0x7e204000 word_temp
    cb[13].ti = DMA_TI_WAIT_RESP;
    cb[13].source_ad = ARM_TO_DMA_BUS(&word_temp) + 0;
    cb[13].dest_ad = 0x7e204000 + 0;
    cb[13].txfr_len = 4;
    cb[13].nextconbk = ARM_TO_DMA_BUS(&cb[14]);

    // 1030: mov word_temp.0 #96
    cb[14].ti = DMA_TI_WAIT_RESP;
    cb[14].source_ad = ARM_TO_DMA_BUS(lut_imm) + 96;
    cb[14].dest_ad = ARM_TO_DMA_BUS(&word_temp) + 0;
    cb[14].txfr_len = 1;
    cb[14].nextconbk = ARM_TO_DMA_BUS(&cb[15]);

    // 1031: mov word_temp.1 #0
    cb[15].ti = DMA_TI_WAIT_RESP;
    cb[15].source_ad = ARM_TO_DMA_BUS(lut_imm) + 0;
    cb[15].dest_ad = ARM_TO_DMA_BUS(&word_temp) + 1;
    cb[15].txfr_len = 1;
    cb[15].nextconbk = ARM_TO_DMA_BUS(&cb[16]);

    // 1032: mov word_temp.2 #0
    cb[16].ti = DMA_TI_WAIT_RESP;
    cb[16].source_ad = ARM_TO_DMA_BUS(lut_imm) + 0;
    cb[16].dest_ad = ARM_TO_DMA_BUS(&word_temp) + 2;
    cb[16].txfr_len = 1;
    cb[16].nextconbk = ARM_TO_DMA_BUS(&cb[17]);

    // 1033: mov word_temp.3 #0
    cb[17].ti = DMA_TI_WAIT_RESP;
    cb[17].source_ad = ARM_TO_DMA_BUS(lut_imm) + 0;
    cb[17].dest_ad = ARM_TO_DMA_BUS(&word_temp) + 3;
    cb[17].txfr_len = 1;
    cb[17].nextconbk = ARM_TO_DMA_BUS(&cb[18]);

    // 1034: movw $0x7e20400c word_temp
    cb[18].ti = DMA_TI_WAIT_RESP;
    cb[18].source_ad = ARM_TO_DMA_BUS(&word_temp) + 0;
    cb[18].dest_ad = 0x7e20400c + 0;
    cb[18].txfr_len = 4;
    cb[18].nextconbk = ARM_TO_DMA_BUS(&cb[19]);

    // 1040: movw word_temp $0x7e204000
    cb[19].ti = DMA_TI_WAIT_RESP;
    cb[19].source_ad = 0x7e204000 + 0;
    cb[19].dest_ad = ARM_TO_DMA_BUS(&word_temp) + 0;
    cb[19].txfr_len = 4;
    cb[19].nextconbk = ARM_TO_DMA_BUS(&cb[20]);

    // 1041: mov 1050.1 word_temp
    cb[20].ti = DMA_TI_WAIT_RESP;
    cb[20].source_ad = ARM_TO_DMA_BUS(&word_temp) + 0;
    cb[20].dest_ad = ARM_TO_DMA_BUS(&cb[21].source_ad) + 1;
    cb[20].txfr_len = 1;
    cb[20].nextconbk = ARM_TO_DMA_BUS(&cb[21]);

    // 1050: orr word_temp #0b10000000
    cb[21].ti = DMA_TI_WAIT_RESP;
    cb[21].source_ad = ARM_TO_DMA_BUS(lut_orr) + 0b10000000;
    cb[21].dest_ad = ARM_TO_DMA_BUS(&word_temp) + 0;
    cb[21].txfr_len = 1;
    cb[21].nextconbk = ARM_TO_DMA_BUS(&cb[22]);

    // 1060: mov 1062.1 word_temp.1
    cb[22].ti = DMA_TI_WAIT_RESP;
    cb[22].source_ad = ARM_TO_DMA_BUS(&word_temp) + 1;
    cb[22].dest_ad = ARM_TO_DMA_BUS(&cb[23].source_ad) + 1;
    cb[22].txfr_len = 1;
    cb[22].nextconbk = ARM_TO_DMA_BUS(&cb[23]);

    // 1062: orr word_temp.1 #1
    cb[23].ti = DMA_TI_WAIT_RESP;
    cb[23].source_ad = ARM_TO_DMA_BUS(lut_orr) + 1;
    cb[23].dest_ad = ARM_TO_DMA_BUS(&word_temp) + 1;
    cb[23].txfr_len = 1;
    cb[23].nextconbk = ARM_TO_DMA_BUS(&cb[24]);

    // 1063: movw $0x7e204000 word_temp
    cb[24].ti = DMA_TI_WAIT_RESP;
    cb[24].source_ad = ARM_TO_DMA_BUS(&word_temp) + 0;
    cb[24].dest_ad = 0x7e204000 + 0;
    cb[24].txfr_len = 4;
    cb[24].nextconbk = ARM_TO_DMA_BUS(&cb[25]);

    // 1064: movw $0x7e007104 spi_dma_cb_rx_addr
    cb[25].ti = DMA_TI_WAIT_RESP;
    cb[25].source_ad = ARM_TO_DMA_BUS(&spi_dma_cb_rx_addr) + 0;
    cb[25].dest_ad = 0x7e007104 + 0;
    cb[25].txfr_len = 4;
    cb[25].nextconbk = ARM_TO_DMA_BUS(&cb[26]);

    // 1065: mov $0x7e007100 #3
    cb[26].ti = DMA_TI_WAIT_RESP;
    cb[26].source_ad = ARM_TO_DMA_BUS(lut_imm) + 3;
    cb[26].dest_ad = 0x7e007100 + 0;
    cb[26].txfr_len = 1;
    cb[26].nextconbk = ARM_TO_DMA_BUS(&cb[27]);

    // 1070: jdma spi_dma_cb
    cb[27].ti = DMA_TI_WAIT_RESP;
    cb[27].source_ad = 0;
    cb[27].dest_ad = 0;
    cb[27].txfr_len = 0;
    cb[27].nextconbk = ARM_TO_DMA_BUS(&spi_dma_cb);
    // External DMA block at spi_dma_cb
    spi_dma_cb.nextconbk = ARM_TO_DMA_BUS(&cb[28]);
    // Wire in external DMA block

    // 1080: mov 1090.1 $0x7e204002
    cb[28].ti = DMA_TI_WAIT_RESP;
    cb[28].source_ad = 0x7e204002 + 0;
    cb[28].dest_ad = ARM_TO_DMA_BUS(&cb[29].source_ad) + 1;
    cb[28].txfr_len = 1;
    cb[28].nextconbk = ARM_TO_DMA_BUS(&cb[29]);

    // 1090: and 1100 #1
    cb[29].ti = DMA_TI_WAIT_RESP;
    cb[29].source_ad = ARM_TO_DMA_BUS(lut_and) + 1;
    cb[29].dest_ad = ARM_TO_DMA_BUS(&cb[30].source_ad) + 0;
    cb[29].txfr_len = 1;
    cb[29].nextconbk = ARM_TO_DMA_BUS(&cb[30]);

    // 1100: tez 1080
    cb[30].ti = DMA_TI_WAIT_RESP;
    cb[30].source_ad = ARM_TO_DMA_BUS(lut_tez);
    cb[30].dest_ad = ARM_TO_DMA_BUS(&cb[31].source_ad);
    cb[30].txfr_len = 1;
    cb[30].nextconbk = ARM_TO_DMA_BUS(&cb[31]);

    // 1100//lda: lda 1080
    cb[31].ti = DMA_TI_WAIT_RESP;
    cb[31].source_ad = ARM_TO_DMA_BUS(&lda_1100);
    cb[31].dest_ad = ARM_TO_DMA_BUS(&cb[32].nextconbk) + 0;
    cb[31].txfr_len = 4;
    cb[31].nextconbk = ARM_TO_DMA_BUS(&cb[32]);

    // 1100//jmp: j 1080
    cb[32].ti = DMA_TI_WAIT_RESP;
    cb[32].source_ad = 0;
    cb[32].dest_ad = 0;
    cb[32].txfr_len = 0;
    cb[32].nextconbk = ARM_TO_DMA_BUS(&cb[28]);

    // 1110: movw word_temp $0x7e204000
    cb[33].ti = DMA_TI_WAIT_RESP;
    cb[33].source_ad = 0x7e204000 + 0;
    cb[33].dest_ad = ARM_TO_DMA_BUS(&word_temp) + 0;
    cb[33].txfr_len = 4;
    cb[33].nextconbk = ARM_TO_DMA_BUS(&cb[34]);

    // 1111: mov 1120.1 word_temp
    cb[34].ti = DMA_TI_WAIT_RESP;
    cb[34].source_ad = ARM_TO_DMA_BUS(&word_temp) + 0;
    cb[34].dest_ad = ARM_TO_DMA_BUS(&cb[35].source_ad) + 1;
    cb[34].txfr_len = 1;
    cb[34].nextconbk = ARM_TO_DMA_BUS(&cb[35]);

    // 1120: and word_temp #0b01111111
    cb[35].ti = DMA_TI_WAIT_RESP;
    cb[35].source_ad = ARM_TO_DMA_BUS(lut_and) + 0b01111111;
    cb[35].dest_ad = ARM_TO_DMA_BUS(&word_temp) + 0;
    cb[35].txfr_len = 1;
    cb[35].nextconbk = ARM_TO_DMA_BUS(&cb[36]);

    // 1130: movw $0x7e204000 word_temp
    cb[36].ti = DMA_TI_WAIT_RESP;
    cb[36].source_ad = ARM_TO_DMA_BUS(&word_temp) + 0;
    cb[36].dest_ad = 0x7e204000 + 0;
    cb[36].txfr_len = 4;
    cb[36].nextconbk = ARM_TO_DMA_BUS(&cb[37]);

    // 2000: mov word_temp.0 #0
    cb[37].ti = DMA_TI_WAIT_RESP;
    cb[37].source_ad = ARM_TO_DMA_BUS(lut_imm) + 0;
    cb[37].dest_ad = ARM_TO_DMA_BUS(&word_temp) + 0;
    cb[37].txfr_len = 1;
    cb[37].nextconbk = ARM_TO_DMA_BUS(&cb[38]);

    // 2001: mov word_temp.1 #0
    cb[38].ti = DMA_TI_WAIT_RESP;
    cb[38].source_ad = ARM_TO_DMA_BUS(lut_imm) + 0;
    cb[38].dest_ad = ARM_TO_DMA_BUS(&word_temp) + 1;
    cb[38].txfr_len = 1;
    cb[38].nextconbk = ARM_TO_DMA_BUS(&cb[39]);

    // 2002: mov word_temp.2 #0
    cb[39].ti = DMA_TI_WAIT_RESP;
    cb[39].source_ad = ARM_TO_DMA_BUS(lut_imm) + 0;
    cb[39].dest_ad = ARM_TO_DMA_BUS(&word_temp) + 2;
    cb[39].txfr_len = 1;
    cb[39].nextconbk = ARM_TO_DMA_BUS(&cb[40]);

    // 2003: mov word_temp.3 #1
    cb[40].ti = DMA_TI_WAIT_RESP;
    cb[40].source_ad = ARM_TO_DMA_BUS(lut_imm) + 1;
    cb[40].dest_ad = ARM_TO_DMA_BUS(&word_temp) + 3;
    cb[40].txfr_len = 1;
    cb[40].nextconbk = ARM_TO_DMA_BUS(&cb[41]);

    // 2004: movw $0x7e20001C word_temp
    cb[41].ti = DMA_TI_WAIT_RESP;
    cb[41].source_ad = ARM_TO_DMA_BUS(&word_temp) + 0;
    cb[41].dest_ad = 0x7e20001C + 0;
    cb[41].txfr_len = 4;
    cb[41].nextconbk = ARM_TO_DMA_BUS(&cb[42]);

    // 1c10: mov e $0x7e003004
    cb[42].ti = DMA_TI_WAIT_RESP;
    cb[42].source_ad = 0x7e003004 + 0;
    cb[42].dest_ad = ARM_TO_DMA_BUS(&e) + 0;
    cb[42].txfr_len = 1;
    cb[42].nextconbk = ARM_TO_DMA_BUS(&cb[43]);

    // 1c20: mov 1c40.1 e
    cb[43].ti = DMA_TI_WAIT_RESP;
    cb[43].source_ad = ARM_TO_DMA_BUS(&e) + 0;
    cb[43].dest_ad = ARM_TO_DMA_BUS(&cb[45].source_ad) + 1;
    cb[43].txfr_len = 1;
    cb[43].nextconbk = ARM_TO_DMA_BUS(&cb[44]);

    // 1c30: mov 1c40.0 $0x7e003004
    cb[44].ti = DMA_TI_WAIT_RESP;
    cb[44].source_ad = 0x7e003004 + 0;
    cb[44].dest_ad = ARM_TO_DMA_BUS(&cb[45].source_ad) + 0;
    cb[44].txfr_len = 1;
    cb[44].nextconbk = ARM_TO_DMA_BUS(&cb[45]);

    // 1c40: sub 1c50
    cb[45].ti = DMA_TI_WAIT_RESP;
    cb[45].source_ad = ARM_TO_DMA_BUS(lut_sub);
    cb[45].dest_ad = ARM_TO_DMA_BUS(&cb[46].source_ad) + 0;
    cb[45].txfr_len = 1;
    cb[45].nextconbk = ARM_TO_DMA_BUS(&cb[46]);

    // 1c50: tez 1c20
    cb[46].ti = DMA_TI_WAIT_RESP;
    cb[46].source_ad = ARM_TO_DMA_BUS(lut_tez);
    cb[46].dest_ad = ARM_TO_DMA_BUS(&cb[47].source_ad);
    cb[46].txfr_len = 1;
    cb[46].nextconbk = ARM_TO_DMA_BUS(&cb[47]);

    // 1c50//lda: lda 1c20
    cb[47].ti = DMA_TI_WAIT_RESP;
    cb[47].source_ad = ARM_TO_DMA_BUS(&lda_1c50);
    cb[47].dest_ad = ARM_TO_DMA_BUS(&cb[48].nextconbk) + 0;
    cb[47].txfr_len = 4;
    cb[47].nextconbk = ARM_TO_DMA_BUS(&cb[48]);

    // 1c50//jmp: j 1c20
    cb[48].ti = DMA_TI_WAIT_RESP;
    cb[48].source_ad = 0;
    cb[48].dest_ad = 0;
    cb[48].txfr_len = 0;
    cb[48].nextconbk = ARM_TO_DMA_BUS(&cb[43]);

    // 2010: movw $0x7e200028 word_temp
    cb[49].ti = DMA_TI_WAIT_RESP;
    cb[49].source_ad = ARM_TO_DMA_BUS(&word_temp) + 0;
    cb[49].dest_ad = 0x7e200028 + 0;
    cb[49].txfr_len = 4;
    cb[49].nextconbk = ARM_TO_DMA_BUS(&cb[50]);

    // 2c10: mov e $0x7e003004
    cb[50].ti = DMA_TI_WAIT_RESP;
    cb[50].source_ad = 0x7e003004 + 0;
    cb[50].dest_ad = ARM_TO_DMA_BUS(&e) + 0;
    cb[50].txfr_len = 1;
    cb[50].nextconbk = ARM_TO_DMA_BUS(&cb[51]);

    // 2c20: mov 2c40.1 e
    cb[51].ti = DMA_TI_WAIT_RESP;
    cb[51].source_ad = ARM_TO_DMA_BUS(&e) + 0;
    cb[51].dest_ad = ARM_TO_DMA_BUS(&cb[53].source_ad) + 1;
    cb[51].txfr_len = 1;
    cb[51].nextconbk = ARM_TO_DMA_BUS(&cb[52]);

    // 2c30: mov 2c40.0 $0x7e003004
    cb[52].ti = DMA_TI_WAIT_RESP;
    cb[52].source_ad = 0x7e003004 + 0;
    cb[52].dest_ad = ARM_TO_DMA_BUS(&cb[53].source_ad) + 0;
    cb[52].txfr_len = 1;
    cb[52].nextconbk = ARM_TO_DMA_BUS(&cb[53]);

    // 2c40: sub 2c50
    cb[53].ti = DMA_TI_WAIT_RESP;
    cb[53].source_ad = ARM_TO_DMA_BUS(lut_sub);
    cb[53].dest_ad = ARM_TO_DMA_BUS(&cb[54].source_ad) + 0;
    cb[53].txfr_len = 1;
    cb[53].nextconbk = ARM_TO_DMA_BUS(&cb[54]);

    // 2c50: tez 2c20
    cb[54].ti = DMA_TI_WAIT_RESP;
    cb[54].source_ad = ARM_TO_DMA_BUS(lut_tez);
    cb[54].dest_ad = ARM_TO_DMA_BUS(&cb[55].source_ad);
    cb[54].txfr_len = 1;
    cb[54].nextconbk = ARM_TO_DMA_BUS(&cb[55]);

    // 2c50//lda: lda 2c20
    cb[55].ti = DMA_TI_WAIT_RESP;
    cb[55].source_ad = ARM_TO_DMA_BUS(&lda_2c50);
    cb[55].dest_ad = ARM_TO_DMA_BUS(&cb[56].nextconbk) + 0;
    cb[55].txfr_len = 4;
    cb[55].nextconbk = ARM_TO_DMA_BUS(&cb[56]);

    // 2c50//jmp: j 2c20
    cb[56].ti = DMA_TI_WAIT_RESP;
    cb[56].source_ad = 0;
    cb[56].dest_ad = 0;
    cb[56].txfr_len = 0;
    cb[56].nextconbk = ARM_TO_DMA_BUS(&cb[51]);

    // 2020: mov word_temp.3 #2
    cb[57].ti = DMA_TI_WAIT_RESP;
    cb[57].source_ad = ARM_TO_DMA_BUS(lut_imm) + 2;
    cb[57].dest_ad = ARM_TO_DMA_BUS(&word_temp) + 3;
    cb[57].txfr_len = 1;
    cb[57].nextconbk = ARM_TO_DMA_BUS(&cb[58]);

    // 2021: movw $0x7e20001C word_temp
    cb[58].ti = DMA_TI_WAIT_RESP;
    cb[58].source_ad = ARM_TO_DMA_BUS(&word_temp) + 0;
    cb[58].dest_ad = 0x7e20001C + 0;
    cb[58].txfr_len = 4;
    cb[58].nextconbk = ARM_TO_DMA_BUS(&cb[59]);

    // 3c10: mov e $0x7e003005
    cb[59].ti = DMA_TI_WAIT_RESP;
    cb[59].source_ad = 0x7e003005 + 0;
    cb[59].dest_ad = ARM_TO_DMA_BUS(&e) + 0;
    cb[59].txfr_len = 1;
    cb[59].nextconbk = ARM_TO_DMA_BUS(&cb[60]);

    // 3c20: mov 3c40.1 e
    cb[60].ti = DMA_TI_WAIT_RESP;
    cb[60].source_ad = ARM_TO_DMA_BUS(&e) + 0;
    cb[60].dest_ad = ARM_TO_DMA_BUS(&cb[62].source_ad) + 1;
    cb[60].txfr_len = 1;
    cb[60].nextconbk = ARM_TO_DMA_BUS(&cb[61]);

    // 3c30: mov 3c40.0 $0x7e003005
    cb[61].ti = DMA_TI_WAIT_RESP;
    cb[61].source_ad = 0x7e003005 + 0;
    cb[61].dest_ad = ARM_TO_DMA_BUS(&cb[62].source_ad) + 0;
    cb[61].txfr_len = 1;
    cb[61].nextconbk = ARM_TO_DMA_BUS(&cb[62]);

    // 3c40: sub 3c50
    cb[62].ti = DMA_TI_WAIT_RESP;
    cb[62].source_ad = ARM_TO_DMA_BUS(lut_sub);
    cb[62].dest_ad = ARM_TO_DMA_BUS(&cb[63].source_ad) + 0;
    cb[62].txfr_len = 1;
    cb[62].nextconbk = ARM_TO_DMA_BUS(&cb[63]);

    // 3c50: tez 3c20
    cb[63].ti = DMA_TI_WAIT_RESP;
    cb[63].source_ad = ARM_TO_DMA_BUS(lut_tez);
    cb[63].dest_ad = ARM_TO_DMA_BUS(&cb[64].source_ad);
    cb[63].txfr_len = 1;
    cb[63].nextconbk = ARM_TO_DMA_BUS(&cb[64]);

    // 3c50//lda: lda 3c20
    cb[64].ti = DMA_TI_WAIT_RESP;
    cb[64].source_ad = ARM_TO_DMA_BUS(&lda_3c50);
    cb[64].dest_ad = ARM_TO_DMA_BUS(&cb[65].nextconbk) + 0;
    cb[64].txfr_len = 4;
    cb[64].nextconbk = ARM_TO_DMA_BUS(&cb[65]);

    // 3c50//jmp: j 3c20
    cb[65].ti = DMA_TI_WAIT_RESP;
    cb[65].source_ad = 0;
    cb[65].dest_ad = 0;
    cb[65].txfr_len = 0;
    cb[65].nextconbk = ARM_TO_DMA_BUS(&cb[60]);

    // 2030: movw $0x7e200028 word_temp
    cb[66].ti = DMA_TI_WAIT_RESP;
    cb[66].source_ad = ARM_TO_DMA_BUS(&word_temp) + 0;
    cb[66].dest_ad = 0x7e200028 + 0;
    cb[66].txfr_len = 4;
    cb[66].nextconbk = ARM_TO_DMA_BUS(&cb[67]);

    // 3000: mov 3010.1 row
    cb[67].ti = DMA_TI_WAIT_RESP;
    cb[67].source_ad = ARM_TO_DMA_BUS(&row) + 0;
    cb[67].dest_ad = ARM_TO_DMA_BUS(&cb[68].source_ad) + 1;
    cb[67].txfr_len = 1;
    cb[67].nextconbk = ARM_TO_DMA_BUS(&cb[68]);

    // 3010: add 3020.1 #1
    cb[68].ti = DMA_TI_WAIT_RESP;
    cb[68].source_ad = ARM_TO_DMA_BUS(lut_add) + 1;
    cb[68].dest_ad = ARM_TO_DMA_BUS(&cb[69].source_ad) + 1;
    cb[68].txfr_len = 1;
    cb[68].nextconbk = ARM_TO_DMA_BUS(&cb[69]);

    // 3020: and 3030 #3
    cb[69].ti = DMA_TI_WAIT_RESP;
    cb[69].source_ad = ARM_TO_DMA_BUS(lut_and) + 3;
    cb[69].dest_ad = ARM_TO_DMA_BUS(&cb[70].source_ad) + 0;
    cb[69].txfr_len = 1;
    cb[69].nextconbk = ARM_TO_DMA_BUS(&cb[70]);

    // 3030: mov row
    cb[70].ti = DMA_TI_WAIT_RESP;
    cb[70].source_ad = ARM_TO_DMA_BUS(lut_imm);
    cb[70].dest_ad = ARM_TO_DMA_BUS(&row) + 0;
    cb[70].txfr_len = 1;
    cb[70].nextconbk = ARM_TO_DMA_BUS(&cb[71]);

    // 4000: j 0000 @ loop
    cb[71].ti = DMA_TI_WAIT_RESP;
    cb[71].source_ad = 0;
    cb[71].dest_ad = 0;
    cb[71].txfr_len = 0;
    cb[71].nextconbk = ARM_TO_DMA_BUS(&cb[0]);

    // 114514: jdma p10_offset_table
    cb[72].ti = DMA_TI_WAIT_RESP;
    cb[72].source_ad = 0;
    cb[72].dest_ad = 0;
    cb[72].txfr_len = 0;
    cb[72].nextconbk = ARM_TO_DMA_BUS(&p10_offset_table);
    // External DMA block at p10_offset_table
    p10_offset_table.nextconbk = ARM_TO_DMA_BUS(&cb[73]);
    // Wire in external DMA block

    // 114515: jdma spi_dma_cb_rx_addr
    cb[73].ti = DMA_TI_WAIT_RESP;
    cb[73].source_ad = 0;
    cb[73].dest_ad = 0;
    cb[73].txfr_len = 0;
    cb[73].nextconbk = ARM_TO_DMA_BUS(&spi_dma_cb_rx_addr);
    // External DMA block at spi_dma_cb_rx_addr
    spi_dma_cb_rx_addr.nextconbk = ARM_TO_DMA_BUS(&cb[74]);
    // Wire in external DMA block

    // 13030: mov row
    cb[74].ti = DMA_TI_WAIT_RESP;
    cb[74].source_ad = ARM_TO_DMA_BUS(lut_imm);
    cb[74].dest_ad = ARM_TO_DMA_BUS(&row) + 0;
    cb[74].txfr_len = 1;
    cb[74].nextconbk = 0;

    MK_REG(DMA_CONBLK_AD_REG(0)) = ARM_TO_DMA_BUS(&cb[0]);
    MK_REG(DMA_CS_REG(0)) = DMA_CS_ACTIVE;
    //while (MK_REG(DMA_CS_REG(0)) & DMA_CS_ACTIVE)
    //    ;
    
    printk("e = %d\n", e);
    printk("row = %d\n", row);
}
