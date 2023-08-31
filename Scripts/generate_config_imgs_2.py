import os
import sys
from intelhex import bin2hex

from header_constants import (
    FLASH_PAGE_SIZE,
    FLASH_CONFIG_DATA_START_ADDR,
    FLASH_CONFIG_DATA_END_ADDR,
)

THIS_DIR      = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT_DIR = os.path.dirname(THIS_DIR)
BIN_DIR       = os.path.join(REPO_ROOT_DIR, "Config_Images")

PB_OUT_DIR = os.path.join(REPO_ROOT_DIR, "Protobufs", "AutoGenerated", "Py")
sys.path.append(PB_OUT_DIR)

import flash_config_data_pb2 as pb

# in case we want this in the future
def crc32(data: bytes) -> int:
    crc = 0xFFFFFFFF
    for byte in data:
        crc ^= byte

        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0xEDB88320
            else:
                crc = (crc >> 1) ^ 0

    return crc ^ 0xFFFFFFFF


def pad_hostname(hostname: bytes) -> bytes:
    assert(len(hostname) <= pb.FlashDataConfig_FieldLen.Hostname_len)
    return hostname + bytes([0xFF] * (pb.FlashDataConfig_FieldLen.Hostname_len - len(hostname)))


def generate_anchor_config(anchor_id: int):
    #########################################################################
    swap_count = 0

    fw_update_pending = 0

    socket_recv_timeout_ms = 5000

    mac_addr = [0x00, 0x08, 0xdc, 0x00, 0xab, anchor_id]
    using_dhcp = 1

    static_ip_addr = [192, 168, 8, 3 + anchor_id]
    static_netmask = [255, 255, 255, 0]
    static_gateway = [192, 168, 8, 1]

    server_hostname = [
        b"GuidingLight._mqtt._tcp.local.",
        bytes([0xFF] * 128),
        bytes([0xFF] * 128),
        bytes([0xFF] * 128),
        bytes([0xFF] * 128),
        bytes([0xFF] * 128),
        bytes([0xFF] * 128),
        bytes([0xFF] * 128),
        bytes([0xFF] * 128),
        bytes([0xFF] * 128)
    ]
    server_ip_addr = [
        [192, 168, 8, 2],
        [0xFF, 0xFF, 0xFF, 0xFF],
        [0xFF, 0xFF, 0xFF, 0xFF],
        [0xFF, 0xFF, 0xFF, 0xFF],
        [0xFF, 0xFF, 0xFF, 0xFF],
        [0xFF, 0xFF, 0xFF, 0xFF],
        [0xFF, 0xFF, 0xFF, 0xFF],
        [0xFF, 0xFF, 0xFF, 0xFF],
        [0xFF, 0xFF, 0xFF, 0xFF],
        [0xFF, 0xFF, 0xFF, 0xFF]
    ]
    server_port = [
        1883,
        0xFFFFFFFF,
        0xFFFFFFFF,
        0xFFFFFFFF,
        0xFFFFFFFF,
        0xFFFFFFFF,
        0xFFFFFFFF,
        0xFFFFFFFF,
        0xFFFFFFFF,
        0xFFFFFFFF
    ]
    #########################################################################

    #########################################################################
    assert( len(mac_addr) == pb.FlashDataConfig_FieldLen.MacAddress_len )

    assert( len(static_ip_addr) == pb.FlashDataConfig_FieldLen.IPv4Address_len )
    assert( len(static_netmask) == pb.FlashDataConfig_FieldLen.IPv4Address_len )
    assert( len(static_gateway) == pb.FlashDataConfig_FieldLen.IPv4Address_len )

    assert( len(server_hostname) == pb.FlashDataConfig_FieldLen.NumFallbackServers )
    assert( len(server_ip_addr) == pb.FlashDataConfig_FieldLen.NumFallbackServers )
    assert( len(server_port) == pb.FlashDataConfig_FieldLen.NumFallbackServers )

    server_hostname = [ pad_hostname(hostname) for hostname in server_hostname ]

    for hostname in server_hostname:
        assert( len(hostname) == pb.FlashDataConfig_FieldLen.Hostname_len )

    for ip_addr in server_ip_addr:
        assert( len(ip_addr) == pb.FlashDataConfig_FieldLen.IPv4Address_len )
    #########################################################################

    #########################################################################

    mac_addr_ = bytes(mac_addr)

    static_ip_addr_ = bytes(static_ip_addr)
    static_netmask_ = bytes(static_netmask)
    static_gateway_ = bytes(static_gateway)


    # server_hostname_ = [ Hostname.from_bytes(b) for b in server_hostname ]
    # server_hostname_ = (Hostname * NUM_FALLBACK_SERVERS)(*server_hostname_)     # type: ignore

    # server_ip_addr_ = [ Ipv4_Addr.from_list(a) for a in server_ip_addr ]
    # server_ip_addr_ = (Ipv4_Addr * NUM_FALLBACK_SERVERS)(*server_ip_addr_)      # type: ignore

    # server_port_    = (c_uint32 * NUM_FALLBACK_SERVERS)(*server_port)

    flash_config = pb.FlashConfigData()

    flash_config.swap_count             = swap_count
    flash_config.fw_update_pending      = fw_update_pending

    flash_config.anchor_id              = anchor_id

    flash_config.socket_recv_timeout_ms = socket_recv_timeout_ms
    flash_config.mac_addr               = mac_addr_
    flash_config.using_dhcp             = using_dhcp

    flash_config.static_ip_addr.octets  = static_ip_addr_
    flash_config.static_netmask.octets  = static_netmask_
    flash_config.static_gateway.octets  = static_gateway_

    for i in range(pb.FlashDataConfig_FieldLen.NumFallbackServers):
        hostname = pb.Hostname()
        hostname.hostname = server_hostname[i]      # type: ignore

        ip_addr = pb.IPv4Address()
        ip_addr.octets = bytes(server_ip_addr[i])   # type: ignore

        port = server_port[i]

        flash_config.server_hostname.append( hostname )
        flash_config.server_ip_addr.append( ip_addr )
        flash_config.server_port.append( port )

    # Bytes of the struct, minus the uint32_t crc32 field
    flash_config_bytes = flash_config.SerializeToString()[:-4]

    # Calculate the CRC32 of the struct
    flash_config.crc32 = crc32(flash_config_bytes)

    # Bytes to write to flash
    img_bytes = flash_config.SerializeToString()
    print(img_bytes)

    # pad with FFs so it spans 2 pages
    img_bytes += bytes( [0xFF] * (FLASH_CONFIG_DATA_END_ADDR - FLASH_CONFIG_DATA_START_ADDR - len(img_bytes)) )

    # Write to files
    filename_no_ext = os.path.join(BIN_DIR, f"a{anchor_id}")

    bin_filename = filename_no_ext + ".bin"
    hex_filename = filename_no_ext + ".hex"

    with open(bin_filename, "wb") as f_bin:
        f_bin.write( img_bytes )

    bin2hex(bin_filename, hex_filename, FLASH_CONFIG_DATA_START_ADDR)

    print(f"Generated config images {hex_filename}")


def main():
    for i in range(5):
        generate_anchor_config(i)


if __name__ == '__main__':
    main()
