import subprocess
from io import BytesIO

from scapy.layers.inet import IP, TCP, Ether
from scapy.packet import Packet, Raw
from scapy.utils import PcapWriter


def _build_raw(length: int):
    return b"a" * length


def _build_packet(length: int) -> Packet:
    headers = Ether() / IP() / TCP()
    payload_len = length - len(headers.build())
    return headers / Raw(_build_raw(payload_len))


def build_cap(seconds: int, pps: int, kbps: int) -> bytes:
    f = BytesIO()
    pkt_len = int(kbps * 1024 / pps / 8)
    with PcapWriter(f) as writer:
        for i in range(pps * seconds):
            packet = _build_packet(pkt_len)
            packet.time = i / pps
            writer.write(packet)
        return f.getvalue()


def replay_cap(cap: bytes, interface: str) -> None:
    subprocess.run(["sudo", "tcpreplay", "-i", interface, "-"], input=cap)


def is_interface_in_promisc(interface: str) -> bool:
    # TODO: Find a nicer way to do this
    completed_process = subprocess.run(
        ["ip", "-s", "link", "show", "dev", interface], capture_output=True
    )
    lines = completed_process.stdout.splitlines()
    return b"PROMISC" in lines[0]


def get_rx_pkt_count(interface: str) -> int:
    # TODO: Find a nicer way to do this
    completed_process = subprocess.run(
        ["ip", "-s", "link", "show", "dev", interface], capture_output=True
    )
    lines = completed_process.stdout.splitlines()
    [rx_header] = [line for line in lines if line.strip().startswith(b"RX:")]
    rx_stats = lines[lines.index(rx_header) + 1].split()
    return int(rx_stats[1])
