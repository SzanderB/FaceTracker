import cv2
import socket
import struct
import pickle

# Configuration
HOST = "192.168.7.1"  # Listen on all interfaces
PORT = 65432       # Port for socket communication

def main():
    # Initialize socket
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind((HOST, PORT))
    server_socket.listen(1)

    print(f"Listening for connection on {HOST}:{PORT}...")
    conn, addr = server_socket.accept()
    print(f"Connected to {addr}")

    data = b""
    payload_size = struct.calcsize(">L")

    try:
        while True:
            # Receive message size
            while len(data) < payload_size:
                packet = conn.recv(4096)
                if not packet:
                    break
                data += packet
            if not data:
                break

            packed_size = data[:payload_size]
            data = data[payload_size:]
            frame_size = struct.unpack(">L", packed_size)[0]

            # Receive frame data
            while len(data) < frame_size:
                data += conn.recv(4096)
            frame_data = data[:frame_size]
            data = data[frame_size:]

            # Deserialize and display frame
            frame = pickle.loads(frame_data)
            cv2.imshow('Face Tracking', frame)

            if cv2.waitKey(1) & 0xFF == ord('q'):
                break

    except KeyboardInterrupt:
        print("Stopping...")
    finally:
        conn.close()
        server_socket.close()
        cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
