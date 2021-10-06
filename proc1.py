import socket, sys
import pandas as pd
import struct

def request_key(values, sock):
    counter = 0

    for initial_value, n in values:
        # send in ``initial_value'' first
        sock.sendall(struct.pack("!l", initial_value))
        # then send ``n''
        sock.sendall(struct.pack("!l", n))

        # get our key
        answer = sock.recv(64)
        
        data = struct.unpack("!Q", answer)[0]
        if data == 0:
            continue
        
        print(f'Resposta do servidor: {data}')
        counter += 1
        

    print(f'{counter} chaves foram geradas!')
    sock.close()

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print('File name (with extension) must be called as first parameter')
        exit()

    file_name = sys.argv[1]

    df = pd.read_csv(file_name, header=None, delimiter=",")

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect(("127.0.0.1", 20001))
        iterator = zip(df[0], df[1])
        request_key(iterator, sock)
