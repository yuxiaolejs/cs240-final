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
VIDEO_DEV = sys.argv[3] if len(sys.argv) > 3 else "/dev/video2"

WIDTH = 64
HEIGHT = 48
FRAME_SIZE = WIDTH * HEIGHT // 8
FRAME_PERIOD = 1 / 29.97

THRESHOLD = 127

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)


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

    bits = small > THRESHOLD

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