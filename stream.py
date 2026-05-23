import socket
import time
import os

# Define target address and port
UDP_IP = "10.0.10.183"
UDP_PORT = 9800
FRAME_SIZE = 384
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

with open("badapple.bin", "rb") as f:
    while True:
        sock.sendto(f.read(FRAME_SIZE), (UDP_IP, UDP_PORT))
        time.sleep(0.03)
        # time.sleep(0.03)
        print("Sent a frame to the server")
        if f.tell() == os.fstat(f.fileno()).st_size:
            f.seek(0)  # Restart the file when the end is reached
