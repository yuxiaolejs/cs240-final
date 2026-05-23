#!/usr/bin/env python3
"""
Stream a .mid file to the Pi over UDP, in the format expected by p10.c:
  payload = [0x94, status, note]
The W5500 driver delivers an 8-byte header before the payload, so on the wire
we just send the 3 bytes above.

Requires: pip install mido
Usage:
  python3 stream_midi.py [file.mid] [ip] [port]
Defaults: big_badapple.mid, 10.0.10.183, 9800
"""

import socket
import sys
import time

try:
    from mido import MidiFile
except ImportError:
    sys.stderr.write("need mido: pip install mido\n")
    sys.exit(1)

low_notes = [0] * 4
high_notes = [0] * 2


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else "big_badapple.mid"
    ip   = sys.argv[2] if len(sys.argv) > 2 else "10.0.10.183"
    port = int(sys.argv[3]) if len(sys.argv) > 3 else 9800

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    mid  = MidiFile(path)
    print(f"loaded {path}: {mid.length:.1f}s, {len(mid.tracks)} tracks")
    print(f"streaming to {ip}:{port}")

    sent = 0
    is_on = 1
    try:
        # mid.play() is a real-time generator: it sleeps between events
        # internally so messages come out paced to wall-clock time.
        for msg in mid.play():
            # if msg.channel != 3:
            #     continue  # ignore events not on channel 3 (p10.c only listens to channel 3)
            if msg.type == "note_on" and msg.velocity > 0:
                # is this high? by A4
                is_high = msg.note >= 60
                found = -1
                if not is_high:
                    for i in range(len(low_notes)):
                        if low_notes[i] == 0:
                            low_notes[i] = msg.note
                            found = i
                            break
                else:
                    for i in range(len(high_notes)):
                        if high_notes[i] == 0:
                            high_notes[i] = msg.note
                            found = i + len(low_notes)
                            break
                if found == -1:
                    print(f"warning: no free {'high' if is_high else 'low'} note slots for MIDI note {msg.note}")
                    continue
                status = 0x90 | (found & 0x0F)
                sock.sendto(bytes([0x94, status, msg.note]), (ip, port))
                sent += 1
            elif msg.type == "note_off" or (msg.type == "note_on" and msg.velocity == 0):
                is_high = msg.note >= 60
                found = -1
                if not is_high:
                    for i in range(len(low_notes)):
                        if low_notes[i] == msg.note:
                            low_notes[i] = 0
                            found = i
                            break
                else:
                    for i in range(len(high_notes)):
                        if high_notes[i] == msg.note:
                            high_notes[i] = 0
                            found = i + len(low_notes)
                            break
                if found == -1:
                    print(f"warning: note-off for unknown {'high' if is_high else 'low'} note {msg.note}")
                    continue
                status = 0x80 | (found & 0x0F)
                sock.sendto(bytes([0x94, status, msg.note]), (ip, port))
                sent += 1
            # other event types (program change, control change, meta, etc.)
            # are silently dropped — p10.c's midi_parse only handles note on/off.
            if sent % 32 == 0:
                print(f"  sent {sent} events ({msg})", flush=True)
    except KeyboardInterrupt:
        print("\ninterrupted")
    finally:
        # send note-off for every channel/note to silence any stuck notes
        print("sending all-notes-off…")
        for ch in range(16):
            for note in range(128):
                sock.sendto(bytes([0x94, 0x80 | ch, note]), (ip, port))
                time.sleep(0.0001)
        print(f"done ({sent} note events sent)")


if __name__ == "__main__":
    main()
