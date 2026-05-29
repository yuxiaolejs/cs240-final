static_tables = """
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
"""

table_inits = """
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
"""

import ast
from dataclasses import dataclass, field
import random
import sys
from typing import List, Dict, Optional, Tuple


@dataclass
class CB:
    ti: str
    source_ad: str
    dest_ad: str
    txfr_len: int
    op: str
    args: List[str]
    nextconbk: Optional[str] = None
    comment: str = ""
    extra_line: Optional[str] = None


def parse_input(lines):
    output = {}
    for line in lines:
        line = line.strip()
        if line.startswith("@"):  # comment
            continue
        if not line or line.startswith("#"):
            continue
        if ":" in line:
            label, rest = line.split(":", 1)
            label = label.strip()
            rest = rest.strip()
        else:
            label = None
            rest = line
        parts = rest.split()
        op = parts[0]
        args = [a.rstrip(",") for a in parts[1:]]
        if op not in ("bez", "bnz"):
            output[label] = CB(
                ti="",
                source_ad="",
                dest_ad="",
                txfr_len=0,
                nextconbk=None,
                comment="",
                op=op,
                args=args,
            )
        else:
            # we need three blocks for cond branch,
            output[f"{label}"] = CB(
                ti="",
                source_ad="",
                dest_ad="",
                txfr_len=0,
                nextconbk=None,
                comment="",
                op=f"t{op[1:]}",
                args=args,
            )
            output[f"{label}//lda"] = CB(
                ti="",
                source_ad="",
                dest_ad="",
                txfr_len=0,
                nextconbk=None,
                comment="",
                op="lda",
                args=args,
            )
            output[f"{label}//jmp"] = CB(
                ti="",
                source_ad="",
                dest_ad="",
                txfr_len=0,
                nextconbk=None,
                comment="",
                op="j",
                args=args,
            )
    return output


def assemble(cbs: Dict[str, CB]):
    global static_tables
    symbols = set()
    w_symbols = set()
    b_labels = {}

    black_listed_symbols = set()

    def find_idx_by_label(label: str):
        return list(cbs.keys()).index(label)

    def get_arg(arg: str, position: str = "source_ad", expect_word=False):
        side = 0
        if "." in arg:
            arg, side = arg.split(".")
        if arg.startswith("#"):  # immediate
            return f"ARM_TO_DMA_BUS(lut_imm) + {arg[1:]}"
        elif arg.startswith("$"):  # immediate address (raw, assume bus)
            return f"{arg[1:]} + {side}"
        elif arg in cbs:  # label
            if expect_word:
                raise ValueError(f"label {arg} cannot be used as a word argument")
            # find index of label
            idx = find_idx_by_label(arg)
            return f"ARM_TO_DMA_BUS(&cb[{idx}].{position}) + {side}"
        else:  # variable
            if expect_word:
                w_symbols.add(arg)
            else:
                symbols.add(arg)
            return f"ARM_TO_DMA_BUS(&{arg}) + {side}"

    idx = 0
    for label, cb in cbs.items():
        # source is determined by op / args for imm, dest by get_arg
        EXP_WORD = cb.op == "movw"
        cb.ti = "DMA_TI_WAIT_RESP"
        cb.dest_ad = get_arg(cb.args[0], expect_word=EXP_WORD)
        cb.nextconbk = f"ARM_TO_DMA_BUS(&cb[{idx + 1}])" if idx + 1 < len(cbs) else "0"
        if cb.op in ("add", "mult", "and", "rsh", "lsh", "sub", "orr", "xor"):
            cb.source_ad = f"ARM_TO_DMA_BUS(lut_{cb.op})"
            if len(cb.args) > 1:
                if not cb.args[1].startswith("#"):
                    raise ValueError(f"second argument to {cb.op} must be an immediate")
                cb.source_ad += f" + {cb.args[1][1:]}"
            cb.txfr_len = 1
        elif cb.op in ("tez", "tnz"):
            cb.source_ad = f"ARM_TO_DMA_BUS(lut_{cb.op})"
            cb.dest_ad = (
                f"ARM_TO_DMA_BUS(&cb[{idx + 1}].source_ad)"  # we assume this exists
            )
            cb.txfr_len = 1
        elif cb.op in ("mov", "movw"):
            if len(cb.args) == 1:
                cb.source_ad = f"ARM_TO_DMA_BUS(lut_imm)"
            else:
                cb.source_ad = get_arg(cb.args[1])
            cb.txfr_len = 4 if EXP_WORD else 1
        elif cb.op == "j":
            cb.source_ad = 0
            cb.dest_ad = 0
            cb.txfr_len = 0
            cb.nextconbk = f"ARM_TO_DMA_BUS(&cb[{find_idx_by_label(cb.args[0])}])"
        elif cb.op == "jdma": # jump to an external DMA block, takes in a static label
            cb.source_ad = 0
            cb.dest_ad = 0
            cb.txfr_len = 0
            cb.extra_line = f"    // External DMA block at {cb.args[0]}\n    {cb.args[0]}.nextconbk = {cb.nextconbk};\n    // Wire in external DMA block\n"
            cb.nextconbk = f"ARM_TO_DMA_BUS(&{cb.args[0]})"
            static_tables += f"extern dma_cb_t {cb.args[0]} __attribute__((aligned(32)));\n"
            black_listed_symbols.add(cb.args[0])  # in case it was added as a symbol
        elif cb.op == "lda":
            lda_name = "lda_" + label.split("//")[0]
            cb.source_ad = f"ARM_TO_DMA_BUS(&{lda_name})"
            cb.txfr_len = 4  # copy address
            cb.dest_ad = get_arg(label.replace("//lda", "//jmp"), position="nextconbk")
            b_labels[lda_name] = (
                f"&cb[{idx + 2}]" if idx + 2 < len(cbs) else "0",
                f"&cb[{find_idx_by_label(cb.args[0])}]",
            )
        else:
            raise ValueError(f"unknown op {cb.op}")
        idx += 1
    return symbols - w_symbols - black_listed_symbols, w_symbols - black_listed_symbols, b_labels


def generate_c(cbs: Dict[str, CB], symbols: set, w_symbols: set, b_labels: Dict[str, tuple]):
    print(static_tables)
    counter = 0
    print(f"static dma_cb_t cb[{len(cbs)}] __attribute__((aligned(32)));\n")
    for lda in b_labels:
        print(f"static uint32_t {lda}[2] __attribute__((aligned(256)));")
    print(f"uint8_t {', '.join(symbols)};\n")
    print(f"uint32_t {', '.join(w_symbols)};\n")
    print(f"\nvoid run_dma()\n{{\n")
    print(table_inits)
    print()
    print("    // Initialize branch labels")
    for lda, (jmp_label, target_label) in b_labels.items():
        print(f"    {lda}[0] = ARM_TO_DMA_BUS({jmp_label});")
        print(f"    {lda}[1] = ARM_TO_DMA_BUS({target_label});")
    print()
    for label, cb in cbs.items():
        print(f"    // {label}: {cb.op} {' '.join(cb.args)}")
        print(f"    cb[{counter}].ti = {cb.ti};")
        print(f"    cb[{counter}].source_ad = {cb.source_ad};")
        print(f"    cb[{counter}].dest_ad = {cb.dest_ad};")
        print(f"    cb[{counter}].txfr_len = {cb.txfr_len};")
        print(f"    cb[{counter}].nextconbk = {cb.nextconbk};")
        if cb.extra_line:
            print(cb.extra_line, end="")
        print()
        counter += 1
    print("""    MK_REG(DMA_CONBLK_AD_REG(0)) = ARM_TO_DMA_BUS(&cb[0]);
    MK_REG(DMA_CS_REG(0)) = DMA_CS_ACTIVE;
    //while (MK_REG(DMA_CS_REG(0)) & DMA_CS_ACTIVE)
    //    ;
    """)
    for sym in symbols:
        print(f'    printk("{sym} = %d\\n", {sym});')
    print("}")


if __name__ == "__main__":
    with open(sys.argv[1]) as f:
        lines = f.read().splitlines()
    cbs = parse_input(lines)
    symbols, w_symbols, b_labels = assemble(cbs)
    generate_c(cbs, symbols, w_symbols, b_labels)
