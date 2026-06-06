#!/usr/bin/env python3
"""
Stream badapple.bin frames AND big_badapple.mid MIDI to the Pi over UDP.
One UDP socket, two threads: frames at ~30 fps, MIDI paced by the file.
Both start simultaneously so video and audio stay in sync.

Requires: pip install mido
Usage:   python3 stream_both.py [ip] [port]
Default: 10.0.10.183 9800
"""

import os
import socket
import sys
import threading
import time

try:
    from mido import MidiFile
except ImportError:
    sys.stderr.write("need mido: pip install mido\n")
    sys.exit(1)


UDP_IP = sys.argv[1] if len(sys.argv) > 1 else "10.0.10.183"
UDP_PORT = int(sys.argv[2]) if len(sys.argv) > 2 else 9800
FRAME_SIZE = 384
FRAME_PERIOD = 1 / 29.97
FRAME_FILE = "badapple.bin"
MIDI_FILE = "dong-fangbad-apple-feat-nomico-ver-20.mid"

AUDIO_DELAY = 0.8

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
stop = threading.Event()

low_notes = [0] * 4  # channels 0..3
high_notes = [0] * 2  # channels 4..5


def frame_thread():
    if AUDIO_DELAY < 0:
        print(f"[frames] delaying start by {-AUDIO_DELAY:.1f}s to sync with audio")
        time.sleep(-AUDIO_DELAY)
    print(f"[frames] {FRAME_FILE} @ {1 / FRAME_PERIOD:.0f} fps")
    with open(FRAME_FILE, "rb") as f:
        size = os.fstat(f.fileno()).st_size
        next_tick = time.monotonic()
        frames = 0
        while not stop.is_set():
            data = f.read(FRAME_SIZE)
            if not data:
                f.seek(0)
                continue
            sock.sendto(bytes([0x93]) + data, (UDP_IP, UDP_PORT))
            frames += 1
            if frames % 30 == 0:
                print(f"[frames] {frames}")
            next_tick += FRAME_PERIOD
            delay = next_tick - time.monotonic()
            if delay > 0:
                time.sleep(delay)
            else:
                # we fell behind — reset the clock so we don't burst-catch-up
                next_tick = time.monotonic()


def alloc_low(note):
    for i, n in enumerate(low_notes):
        if n == 0:
            low_notes[i] = note
            return i
    return -1


def alloc_high(note):
    for i, n in enumerate(high_notes):
        if n == 0:
            high_notes[i] = note
            return i + len(low_notes)
    return -1


def free_low(note):
    for i, n in enumerate(low_notes):
        if n == note:
            low_notes[i] = 0
            return i
    return -1


def free_high(note):
    for i, n in enumerate(high_notes):
        if n == note:
            high_notes[i] = 0
            return i + len(low_notes)
    return -1


def midi_thread():
    print(f"[midi] {MIDI_FILE}")
    mid = MidiFile(MIDI_FILE)
    print(f"[midi] {mid.length:.1f}s, {len(mid.tracks)} tracks")
    sent = dropped = 0
    if AUDIO_DELAY > 0:
        print(f"[midi] delaying start by {AUDIO_DELAY:.1f}s to sync with audio")
        time.sleep(AUDIO_DELAY)
    for msg in mid.play():
        if stop.is_set():
            break

        is_note_on = msg.type == "note_on" and msg.velocity > 0
        is_note_off = msg.type == "note_off" or (
            msg.type == "note_on" and msg.velocity == 0
        )

        if not (is_note_on or is_note_off):
            continue

        is_high = msg.note >= 60
        if is_note_on:
            ch = alloc_high(msg.note) if is_high else alloc_low(msg.note)
            if ch < 0:
                dropped += 1
                continue
            sock.sendto(bytes([0x94, 0x90 | (ch & 0x0F), msg.note]), (UDP_IP, UDP_PORT))
        else:
            ch = free_high(msg.note) if is_high else free_low(msg.note)
            if ch < 0:
                continue
            sock.sendto(bytes([0x94, 0x80 | (ch & 0x0F), msg.note]), (UDP_IP, UDP_PORT))

        sent += 1
        if sent % 64 == 0:
            print(f"[midi] {sent} sent, {dropped} dropped (no free voice)")

    print(f"[midi] done: {sent} sent, {dropped} dropped")
    stop.set()  # signal frame thread to wind down


def all_notes_off():
    print("[cleanup] all notes off")
    for ch in range(6):
        for note in range(128):
            sock.sendto(bytes([0x94, 0x80 | ch, note]), (UDP_IP, UDP_PORT))
            time.sleep(0.00005)


def main():
    print(f"-> {UDP_IP}:{UDP_PORT}")
    t_f = threading.Thread(target=frame_thread, daemon=True)
    t_m = threading.Thread(target=midi_thread, daemon=True)
    t_f.start()
    t_m.start()
    try:
        while not stop.is_set():
            time.sleep(0.1)
    except KeyboardInterrupt:
        print("\n[cleanup] interrupted")
    stop.set()
    all_notes_off()
    t_f.join(timeout=1)
    t_m.join(timeout=1)
    print("[cleanup] done")


if __name__ == "__main__":
    main()
