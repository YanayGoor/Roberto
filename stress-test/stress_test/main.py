import click
from stress_test.packets import (
    build_cap,
    get_rx_pkt_count,
    is_interface_in_promisc,
    replay_cap,
)

from .comm import connect_to_master, connect_to_slave, recv, recvall, send, sendall

DEFAULT_PORT = 6867


def _parse_slave_addr(slave: str) -> tuple[str, int]:
    parts = slave.split(":", 1)
    return parts[0], int(parts[1]) if len(parts) > 1 else DEFAULT_PORT


@click.command()
@click.option("-s", "--slaves")
@click.option("-d", "--duration", default=10, type=click.INT)
@click.option("--min-kbps", default=1, type=click.INT)
@click.option("--max-kbps", default=1024 * 10, type=click.INT)
@click.option("--pps", default=1000, type=click.INT)
@click.option("--steps", default=10, type=click.INT)
@click.option("--threshold", default=0.9, type=click.FLOAT)
def run_master(
    slaves: str,
    min_kbps: int,
    max_kbps: int,
    steps: int,
    duration: int,
    threshold: float,
    pps: int,
):
    print("starting master ..")
    addrs = {_parse_slave_addr(slave) for slave in slaves.split(",")}
    socks = [connect_to_slave(addr) for addr in addrs]

    for _ in range(steps):
        kbps = int((min_kbps + max_kbps) / 2)

        print(f"preparing {kbps=} ({min_kbps=}, {max_kbps=})")
        sendall(socks, {"kbps": kbps, "duration": duration, "pps": pps})
        assert all(ack == "READY" for ack in recvall(socks))

        print(f"starting {kbps=}")
        sendall(socks, "START")

        received = sum(result["received"] for result in recvall(socks))
        total = len(addrs) * pps * duration
        print(f'received {received} out of {total}')
        if received / total > threshold:
            min_kbps = kbps
        else:
            max_kbps = kbps

    sendall(socks, "")
    print(f"Final result is {min_kbps}-{max_kbps}kbps")


@click.command()
@click.option("-i", "--interface", help="the interface to stress test")
@click.option("--port", default=DEFAULT_PORT, type=click.INT)
def run_slave(interface, port):
    assert is_interface_in_promisc(
        interface
    ), "interface not in promisc, run 'ip link set eth1 promisc on'"
    print("running client ..")
    s = connect_to_master(port)
    print("connected to master.")
    while config := recv(s):
        cap = build_cap(config["duration"], pps=config["pps"], kbps=config["kbps"])
        send(s, "READY")

        assert recv(s) == "START"
        before = get_rx_pkt_count(interface)
        replay_cap(cap, interface)
        after = get_rx_pkt_count(interface)

        print(f"received {after - before} packets")
        send(s, {"received": after - before})


if __name__ == "__main__":
    pass
