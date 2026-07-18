#!/usr/bin/env python3
"""
Stream live /dev/videoX frames to the Pi over UDP.

Expected Pi frame format:
  packet[0] = 0x93
  packet[1:] = 384 bytes, representing 64x48 1bpp pixels

Usage:
  python3 stream_video.py [ip] [port] [video_device]

Defaults:
  ip:           10.0.10.183
  port:         9800
  video_device: /dev/video10
"""

import socket
import sys
import time

import cv2
import numpy as np


UDP_IP = sys.argv[1] if len(sys.argv) > 1 else "10.0.10.183"
UDP_PORT = int(sys.argv[2]) if len(sys.argv) > 2 else 9800
VIDEO_DEV = sys.argv[3] if len(sys.argv) > 3 else "/dev/video4"

WIDTH = 64
HEIGHT = 48
FRAME_SIZE = WIDTH * HEIGHT // 8
FRAME_PERIOD = 1 / 29.97

THRESHOLD = 128

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)


def atkinson_dither(img):
    """
    Atkinson dither a uint8 grayscale image to a bool array.

    Distributes 6/8 of the quantization error to neighbors:
        .  .  X  1  1
        1  1  1  .  .
        .  1  .  .  .
    Each neighbor gets 1/8 of the error; 2/8 is dropped, which is what
    gives Atkinson its characteristic punchy, high-contrast look.
    """
    buf = img.astype(np.float32)
    h, w = buf.shape

    for y in range(h):
        row0 = buf[y]
        row1 = buf[y + 1] if y + 1 < h else None
        row2 = buf[y + 2] if y + 2 < h else None
        for x in range(w):
            old = row0[x]
            new = 255.0 if old >= THRESHOLD else 0.0
            row0[x] = new
            err = (old - new) * (1.0 / 8.0)
            if err == 0.0:
                continue
            if x + 1 < w:
                row0[x + 1] += err
            if x + 2 < w:
                row0[x + 2] += err
            if row1 is not None:
                if x - 1 >= 0:
                    row1[x - 1] += err
                row1[x] += err
                if x + 1 < w:
                    row1[x + 1] += err
            if row2 is not None:
                row2[x] += err

    return buf >= 128.0


def frame_to_1bpp(frame):
    """
    Convert OpenCV BGR frame to 64x48 packed 1bpp.

    Output is 384 bytes.
    Pixel order:
      row-major
      left-to-right
      MSB first inside each byte
    """

    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

    small = cv2.resize(
        gray,
        (WIDTH, HEIGHT),
        interpolation=cv2.INTER_AREA,
    )

    # Stretch contrast so the dither has the full 0..255 range to work with.
    lo, hi = np.percentile(small, (2, 98))
    if hi > lo:
        stretched = np.clip((small.astype(np.float32) - lo) * (255.0 / (hi - lo)), 0, 255)
        small = stretched.astype(np.uint8)

    bits = atkinson_dither(small)

    # Pack 8 horizontal pixels into one byte, MSB first.
    packed = np.packbits(bits, axis=None, bitorder="big")

    return packed.tobytes()


def main():
    print(f"-> {UDP_IP}:{UDP_PORT}")
    print(f"[video] opening {VIDEO_DEV}")

    cap = cv2.VideoCapture(VIDEO_DEV, cv2.CAP_V4L2)

    if not cap.isOpened():
        raise RuntimeError(f"could not open video device: {VIDEO_DEV}")

    # Optional requests. The device may ignore these.
    cap.set(cv2.CAP_PROP_FPS, 30)

    print("[video] streaming 64x48 1bpp @ ~29.97 fps")
    print("[video] press Ctrl+C to stop")

    frames = 0
    next_tick = time.monotonic()

    try:
        while True:
            ret, frame = cap.read()
            if not ret:
                print("[video] failed to read frame")
                time.sleep(0.05)
                continue

            data = frame_to_1bpp(frame)

            if len(data) != FRAME_SIZE:
                print(f"[video] bad frame size: {len(data)}")
                continue

            sock.sendto(bytes([0x93]) + data, (UDP_IP, UDP_PORT))

            frames += 1
            if frames % 30 == 0:
                print(f"[video] {frames}")

            next_tick += FRAME_PERIOD
            delay = next_tick - time.monotonic()

            if delay > 0:
                time.sleep(delay)
            else:
                # If capture/encoding is too slow, don't burst-catch-up.
                next_tick = time.monotonic()

    except KeyboardInterrupt:
        print("\n[cleanup] interrupted")

    finally:
        cap.release()
        sock.close()
        print("[cleanup] done")


if __name__ == "__main__":
    main()