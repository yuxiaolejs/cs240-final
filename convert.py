import cv2
import numpy as np

TARGET_WIDTH = 32 * 2
TARGET_HEIGHT = 16 * 3
RESIZE_METHOD = cv2.INTER_AREA
# RESIZE_METHOD = cv2.INTER_CUBIC


def resize_crop(img):
    h, w = img.shape[:2]
    target = TARGET_WIDTH / TARGET_HEIGHT
    src = w / h
    if src > target:
        new_w = int(h * target)
        img = img[:, (w - new_w) // 2 : (w + new_w) // 2]
    else:
        new_h = int(w / target)
        img = img[(h - new_h) // 2 : (h + new_h) // 2, :]
    return cv2.resize(img, (TARGET_WIDTH, TARGET_HEIGHT), interpolation=RESIZE_METHOD)


def resize_fill(img):
    h, w = img.shape[:2]

    scale = min(TARGET_WIDTH / w, TARGET_HEIGHT / h)
    new_w = int(w * scale)
    new_h = int(h * scale)

    resized = cv2.resize(img, (new_w, new_h), interpolation=RESIZE_METHOD)

    if img.ndim == 2:
        canvas = np.zeros((TARGET_HEIGHT, TARGET_WIDTH), dtype=img.dtype)
    else:
        canvas = np.zeros((TARGET_HEIGHT, TARGET_WIDTH, img.shape[2]), dtype=img.dtype)

    x0 = (TARGET_WIDTH - new_w) // 2
    y0 = (TARGET_HEIGHT - new_h) // 2

    canvas[y0 : y0 + new_h, x0 : x0 + new_w] = resized
    return canvas


cap = cv2.VideoCapture("badapple.mp4")

writer = cv2.VideoWriter(
    "badapple-resizedf.mp4",
    cv2.VideoWriter_fourcc(*"mp4v"),
    cap.get(cv2.CAP_PROP_FPS),
    (TARGET_WIDTH, TARGET_HEIGHT),
)

total_frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
print(f"Total frames: {total_frames}")

count = 0

with open("badapple.bin", "wb") as f:
    while True:
        print(f"Processing frame {count}/{total_frames} ({count/total_frames:.1%})", end="\r")
        count += 1
        ret, frame = cap.read()

        if not ret:
            break
        frame = resize_fill(frame)
        writer.write(frame)
        for row in frame:
            for chunk in [row[i : i + 8] for i in range(0, len(row), 8)]:
                byte = 0
                for i, pixel in enumerate(chunk):
                    r, g, b = pixel
                    if r > 128 or g > 128 or b > 128:
                        byte |= 1 << (7 - i)
                f.write(byte.to_bytes(1, "big"))

cap.release()
cv2.destroyAllWindows()
