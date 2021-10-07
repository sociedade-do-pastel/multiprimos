import socket, sys
import pandas as pd
import struct
import time
from numpy import mean, std

results = []


def request_key(df, sock):
    for x in range(50):
        values = zip(df[0], df[1])
        counter = 0
        start = time.perf_counter()

        for initial_value, n in values:
            # send in ``initial_value'' first
            sock.sendall(struct.pack("!l", initial_value))
            # then send ``n''
            sock.sendall(struct.pack("!l", n))

            # get our key
            answer = sock.recv(64)

            data = struct.unpack("!Q", answer)[0]
            if (time.perf_counter() - start >= 5):
                break
            if data == 0:
                continue

            counter += 1
        results.append(counter)

        # print(f'{counter} chaves foram geradas!')
    sock.close()


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print('File name (with extension) must be called as first parameter')
        exit()

    file_name = sys.argv[1]

    df = pd.read_csv(file_name, header=None, delimiter=",")

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect(("127.0.0.1", 20001))
        request_key(df, sock)

    print()
    for x in enumerate(results):
        print(f"Teste {x[0]+1:2} - \tChaves encontradas: {x[1]}")

    print(f"Média: {mean(results)}")
    print(f"Desvio padrão: {std(results)}")
