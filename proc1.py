import socket, sys
import pandas as pd

def request_key(initial_value, n, sock):
    sock.connect(("127.0.0.1", 65202))
    sock.sendall(bytes((f'{initial_value} {n}').encode('UTF-8')))
    answer = sock.recv(1024).decode()
    print(f'Resposta do servidor: {answer}')
    # print(f'{initial_value} {n}')

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print('File name must be called as first parameter')
        exit()

    file_name = sys.argv[1]

    df = pd.read_excel(file_name, header=None)

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        for initial_value, n in zip(df[0], df[1]):
            request_key(initial_value, n, sock)