import socket
import pickle
import struct

if __name__=="__main__":
    pass
    # Specify IP Host and Port
    HOST = "192.168.7.1"
    PORT = 65432
    try:
        # Connect to socket
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:

        # Specify timeout period

        # Bind port and begin listening for a connection
            s.bind((HOST, PORT))
            s.listen()
            conn, addr = s.accept()
        # Send a random message to the client
            with conn:
                print(f"Connected by {addr}")
                conn.sendall(b"Hello, world")

                while True:
                    data = pickle.loads(conn.recv(1024))
                    # data = int.from_bytes(s.recv(4), byteorder='little')
                    print(data)

    # Close the server
    except KeyboardInterrupt:

        socket.close()
    finally:
        print("Server program terminated.")