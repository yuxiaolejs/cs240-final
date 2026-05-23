#!/usr/bin/env python3
"""
midi_to_floppy.py
Convert a MIDI file into a C header containing a multi-channel event stream
for polyphonic floppy-drive playback.

Each event is (freq, delay_ms, channel):
    freq != 0  -> start playing freq on `channel`
    freq == 0  -> note_off on `channel`
    delay_ms is the delay BEFORE this event fires (MIDI delta-time style).
    The first event's delay_ms is the offset from t=0 (typically 0).

Channels are allocated greedily: when a MIDI note_on arrives we pick the
lowest-indexed free channel. If no channel is free the note is dropped (or
optionally steals the oldest channel — see --steal). The number of channels
is the maximum polyphony the playback engine supports.

Notes outside the configured range are octave-shifted to fit.

Usage:
    pip install mido
    python parse.py song.mid > song.h
    python parse.py song.mid --channels 4 --var tetris > tetris.h
"""

import argparse
import sys
import mido

DEFAULT_LOW = 36
DEFAULT_HIGH = 84

DRUM_CHANNEL = 9  # General MIDI: channel 10 (0-indexed = 9)


def midi_to_freq(note: int) -> float:
    return 440.0 * (2 ** ((note - 69) / 12.0))


def fold_into_range(note: int, low: int, high: int) -> int:
    while note < low:
        note += 12
    while note > high:
        note -= 12
    return note


def parse_midi(filename, skip_drums, low, high, octave_shift,
               num_channels, steal, ignore_channels):
    """Walk the MIDI file and emit (freq_hz, delay_ms_to_next, channel) tuples."""
    mid = mido.MidiFile(filename)

    # First pass: collect raw note-on / note-off events with absolute ms.
    raw = []                     # (time_ms, kind, midi_note, order)
    abs_time = 0.0
    order = 0
    seen_channels = set()        # MIDI channels that produced any note event
    skipped_drum_channels = set()
    skipped_user_channels = set()
    for msg in mid:
        abs_time += msg.time
        if msg.is_meta:
            continue
        ch = getattr(msg, "channel", None)
        if skip_drums and ch == DRUM_CHANNEL:
            if msg.type in ("note_on", "note_off"):
                skipped_drum_channels.add(ch)
            continue
        if ch is not None and ch in ignore_channels:
            if msg.type in ("note_on", "note_off"):
                skipped_user_channels.add(ch)
            continue
        time_ms = int(round(abs_time * 1000))
        if msg.type == "note_on" and msg.velocity > 0:
            raw.append((time_ms, 1, msg.note, order))   # 1 = on
            if ch is not None:
                seen_channels.add(ch)
        elif msg.type == "note_off" or (msg.type == "note_on" and msg.velocity == 0):
            raw.append((time_ms, 0, msg.note, order))   # 0 = off
            if ch is not None:
                seen_channels.add(ch)
        order += 1

    used = sorted(seen_channels)
    extras = []
    if skipped_drum_channels:
        extras.append(f"skipped drum ch {sorted(skipped_drum_channels)}")
    if skipped_user_channels:
        extras.append(f"ignored ch {sorted(skipped_user_channels)}")
    suffix = ("  (" + ", ".join(extras) + ")") if extras else ""
    print(f"// MIDI: {len(used)} channel(s) used: "
          f"{used if used else '(none)'}{suffix}",
          file=sys.stderr)

    # Compute max concurrent notes by sweeping in time order.
    sweep = sorted(raw, key=lambda e: (e[0], e[1], e[3]))  # offs before ons at tie
    cur, peak = 0, 0
    for _, kind, _, _ in sweep:
        cur += 1 if kind == 1 else -1
        if cur > peak:
            peak = cur
    print(f"// MIDI: peak concurrent notes = {peak} "
          f"(set --channels >= {peak} for lossless playback)",
          file=sys.stderr)

    # Sort: by time, with note_off before note_on at the same instant so a
    # channel freed at time T can be reused by a new note also starting at T.
    raw.sort(key=lambda e: (e[0], e[1], e[3]))

    # channel_state[i] = (midi_note, start_time_ms) currently sounding, or None
    channel_state = [None] * num_channels
    out = []                     # (time_ms, freq, channel)
    dropped = 0
    total_on = 0

    for time_ms, kind, midi_note, _ in raw:
        if kind == 1:            # note_on
            total_on += 1
            free = next((i for i in range(num_channels)
                         if channel_state[i] is None), None)
            if free is None:
                if not steal:
                    dropped += 1
                    continue
                # Steal the oldest channel.
                free = min(range(num_channels),
                           key=lambda i: channel_state[i][1])
                out.append((time_ms, 0, free))
            channel_state[free] = (midi_note, time_ms)
            n = fold_into_range(midi_note + 12 * octave_shift, low, high)
            f = int(round(midi_to_freq(n)))
            out.append((time_ms, f, free))
        else:                    # note_off
            for i in range(num_channels):
                if channel_state[i] is not None and channel_state[i][0] == midi_note:
                    channel_state[i] = None
                    out.append((time_ms, 0, i))
                    break

    # Strip redundant note_offs: if the next event on the same channel happens
    # at the same timestamp, the off would just be overwritten — drop it.
    next_same = [None] * len(out)
    last_seen = {}
    for i in range(len(out) - 1, -1, -1):
        c = out[i][2]
        next_same[i] = last_seen.get(c)
        last_seen[c] = i
    out = [ev for i, ev in enumerate(out)
           if not (ev[1] == 0
                   and next_same[i] is not None
                   and out[next_same[i]][0] == ev[0])]

    # Convert absolute timestamps to delay-BEFORE-this-event deltas.
    events = []
    prev_t = 0
    for t, f, c in out:
        events.append((f, t - prev_t, c))
        prev_t = t

    if total_on:
        pct = 100.0 * dropped / total_on
        print(f"// {dropped}/{total_on} note_ons dropped "
              f"({pct:.1f}%) — increase --channels or pass --steal",
              file=sys.stderr)
    return events


def emit_c(events, varname, num_channels):
    total_ms = sum(d for _, d, _ in events)
    n_on = sum(1 for f, _, _ in events if f != 0)
    n_off = sum(1 for f, _, _ in events if f == 0)

    lines = [
        "// Auto-generated by parse.py",
        f"// {n_on} note_on, {n_off} note_off, {num_channels} channels, "
        f"total {total_ms/1000:.1f} s",
        "",
        "#include <stdint.h>",
        "#include <stddef.h>",
        "",
        "// delay_ms = ms to wait BEFORE applying this event (MIDI delta-time)",
        "typedef struct { uint32_t freq; uint32_t delay_ms; uint8_t channel; } note_t;",
        "",
        f"#define {varname}_channels {num_channels}",
        "",
        f"static const note_t {varname}[] = {{",
    ]
    for f, d, c in events:
        lines.append(f"    {{ {f:5d}, {d:6d}, {c:2d} }},")
    lines += [
        "};",
        f"static const size_t {varname}_len = sizeof({varname}) / sizeof({varname}[0]);",
        "",
        "// Plays the song. Assumes you already have:",
        "//   void play_freq(uint8_t channel, uint32_t freq);  // freq==0 -> off",
        "//   void delay_ms(uint32_t ms);",
        f"static void play_{varname}(void) {{",
        f"    for (size_t i = 0; i < {varname}_len; i++) {{",
        f"        if ({varname}[i].delay_ms) delay_ms({varname}[i].delay_ms);",
        f"        play_freq({varname}[i].channel, {varname}[i].freq);",
        f"    }}",
        f"}}",
    ]
    return "\n".join(lines)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("midi_file")
    ap.add_argument("--var", default="song",
                    help="C variable name (default: song)")
    ap.add_argument("--channels", type=int, default=2,
                    help="number of polyphony channels (default 2)")
    ap.add_argument("--low", type=int, default=DEFAULT_LOW,
                    help=f"lowest MIDI note allowed (default {DEFAULT_LOW} = C2)")
    ap.add_argument("--high", type=int, default=DEFAULT_HIGH,
                    help=f"highest MIDI note allowed (default {DEFAULT_HIGH} = C6)")
    ap.add_argument("--include-drums", action="store_true",
                    help="don't skip MIDI channel 10")
    ap.add_argument("--octave-shift", type=int, default=0,
                    help="shift every note by N octaves before range-folding")
    ap.add_argument("--steal", action="store_true",
                    help="steal oldest channel when out of polyphony "
                         "(default: drop the new note)")
    ap.add_argument("--ignore-channels", default="",
                    help="comma-separated MIDI channel numbers (0-15) to skip "
                         "entirely, e.g. --ignore-channels 0,3")
    args = ap.parse_args()

    if args.channels < 1 or args.channels > 255:
        ap.error("--channels must be in [1, 255]")

    try:
        ignore = {int(x) for x in args.ignore_channels.split(",") if x.strip()}
    except ValueError:
        ap.error("--ignore-channels must be a comma-separated list of integers")
    if any(c < 0 or c > 15 for c in ignore):
        ap.error("--ignore-channels values must be in [0, 15]")

    events = parse_midi(args.midi_file,
                        skip_drums=not args.include_drums,
                        low=args.low, high=args.high,
                        octave_shift=args.octave_shift,
                        num_channels=args.channels,
                        steal=args.steal,
                        ignore_channels=ignore)
    if not events:
        print("// no notes extracted — wrong file or all filtered?",
              file=sys.stderr)
        sys.exit(1)
    print(emit_c(events, args.var, args.channels))


if __name__ == "__main__":
    main()
