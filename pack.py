with open("badapple.bin", "rb") as f:
    data = f.read()


def pack_code(file):
    with open(file, "rb") as f:
        code = f.read()

    print(f"Code size: {hex(len(code))} bytes")
    padded_len = ((len(code) + 4095) // 4096) * 4096
    print(f"Padded code size: {hex(padded_len)} bytes")
    padded = code + bytes([0] * (padded_len - len(code)))

    w_data = padded + data[:1_600_000]
    print(f"Total size: {hex(len(w_data))} bytes")
    print(f"Data address: {hex(len(padded) + 0x8000)}")
    return w_data


with open("bb-padded.bin", "wb") as f:
    f.write(pack_code("bb.bin"))

with open("hw-padded.bin", "wb") as f:
    f.write(pack_code("hw.bin"))
