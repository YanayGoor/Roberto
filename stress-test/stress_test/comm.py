import json
from socket import AF_INET, SOCK_STREAM, socket
from typing import Any


def connect_to_slave(addr: tuple[str, int]) -> socket:
    s = socket(AF_INET, SOCK_STREAM)
    s.connect(addr)
    return s


def connect_to_master(port: int) -> socket:
    s = socket(AF_INET, SOCK_STREAM)
    s.bind(("0.0.0.0", port))
    s.listen()
    s, _ = s.accept()
    return s


def send(s: socket, data: Any) -> None:
    serialized = json.dumps(data).encode()
    s.send(len(serialized).to_bytes(length=4, byteorder="little", signed=False))
    s.send(serialized)


def sendall(slaves: list[socket], data) -> None:
    for s in slaves:
        send(s, data)


def recv(s: socket) -> Any:
    length = int.from_bytes(s.recv(4), byteorder="little", signed=False)
    serialized = s.recv(length)
    return json.loads(serialized.decode())


def recvall(socks: list[socket]) -> list[Any]:
    return [recv(s) for s in socks]
