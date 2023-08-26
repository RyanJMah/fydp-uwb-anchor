import time
import socket

HOST = "0.0.0.0"
PORT = 6900

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind((HOST, PORT))
    s.listen()
    conn, addr = s.accept()
    with conn:
        print(f"Connected by {addr}")

        while True:
            data = conn.recv(1024)
            print(data.decode())

        # while True:
        #     conn.send(b"did it get this?")
        #     time.sleep(1)