import os
import sys
import commentjson as json
from ctypes import LittleEndianStructure, c_uint32, c_uint8, c_char
from intelhex import bin2hex
from typing import List

from header_constants import (
    FLASH_PAGE_SIZE,
    FLASH_CONFIG_DATA_START_ADDR,
    FLASH_CONFIG_DATA_END_ADDR,
    NUM_FALLBACK_SERVERS,
    MAX_HOSTNAME_CHARS
)
from Calc_CRC32 import calc_crc32

THIS_DIR      = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT_DIR = os.path.dirname(THIS_DIR)
BIN_DIR       = os.path.join(REPO_ROOT_DIR, "Config_Images")

# Refer to ./Common/lan.h
class Ipv4_Addr(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("addr", c_uint8 * 4)
    ]

    @classmethod
    def from_list(cls, l):
        uint8_arr = (c_uint8 * len(l))(*l)
        return cls( uint8_arr )

# Refer to ./Common/lan.h
class Hostname(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("c", c_char * MAX_HOSTNAME_CHARS)
    ]

    @classmethod
    def from_bytes(cls, b):
        # pad with 0s
        b = b + bytes( [0x00] * (128 - len(b)) )

        # c_arr = (c_char * 128)(*b)

        return cls( b )

# Refer to ./Common/app_flash_storage.h
class FlashConfig(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("swap_count",              c_uint32),

        ("fw_update_pending",       c_uint8),

        ("anchor_id",               c_uint8),

        ("socket_recv_timeout_ms",  c_uint32),

        ("mac_addr",                c_uint8 * 6),
        ("using_dhcp",              c_uint8),

        ("static_ip_addr",          Ipv4_Addr),
        ("static_netmask",          Ipv4_Addr),
        ("static_gateway",          Ipv4_Addr),

        ("server_hostname",         Hostname  * NUM_FALLBACK_SERVERS),
        ("server_ip_addr",          Ipv4_Addr * NUM_FALLBACK_SERVERS),
        ("server_port",             c_uint32  * NUM_FALLBACK_SERVERS),

        ("crc32",                   c_uint32)
    ]


def _pad_empty_fallback_servers(l: List[bytes], padding_bytes: bytes):
    while len(l) < NUM_FALLBACK_SERVERS:
        l.append(padding_bytes)



def generate_anchor_config(anchor_id: int):
    #########################################################################
    # Import common config from json file
    common_json_path = os.path.join(BIN_DIR, "common_config.jsonc")

    with open(common_json_path, "r") as f:
        json_conf = json.load(f)

    swap_count = json_conf["swap_count"]

    fw_update_pending = json_conf["fw_update_pending"]

    socket_recv_timeout_ms = json_conf["socket_recv_timeout_ms"]

    mac_addr      = json_conf["mac_addr_base"]
    mac_addr[-1] += anchor_id

    using_dhcp = json_conf["using_dhcp"]

    static_ip_addr      = json_conf["static_ip_addr_base"]
    static_ip_addr[-1] += anchor_id

    static_netmask = json_conf["static_netmask"]
    static_gateway = json_conf["static_gateway"]


    server_hostname = [ s.encode() for s in json_conf["server_hostname"] ]
    _pad_empty_fallback_servers( server_hostname, bytes([0xFF] * 128) )

    server_ip_addr = json_conf["server_ip_addr"]
    _pad_empty_fallback_servers( server_ip_addr, bytes([0xFF] * 4) )

    server_port = json_conf["server_port"]
    _pad_empty_fallback_servers( server_port, 0xFFFFFFFF )

    #########################################################################

    mac_addr_       = (c_uint8 * 6)(*mac_addr)

    static_ip_addr_ = Ipv4_Addr.from_list(static_ip_addr)
    static_netmask_ = Ipv4_Addr.from_list(static_netmask)
    static_gateway_ = Ipv4_Addr.from_list(static_gateway)

    server_hostname_ = [ Hostname.from_bytes(b) for b in server_hostname ]
    server_hostname_ = (Hostname * NUM_FALLBACK_SERVERS)(*server_hostname_)     # type: ignore

    server_ip_addr_ = [ Ipv4_Addr.from_list(a) for a in server_ip_addr ]
    server_ip_addr_ = (Ipv4_Addr * NUM_FALLBACK_SERVERS)(*server_ip_addr_)      # type: ignore

    server_port_    = (c_uint32 * NUM_FALLBACK_SERVERS)(*server_port)

    flash_config = FlashConfig( swap_count,
                                fw_update_pending,
                                anchor_id,
                                socket_recv_timeout_ms,
                                mac_addr_,
                                using_dhcp,
                                static_ip_addr_,
                                static_netmask_,
                                static_gateway_,
                                server_hostname_,
                                server_ip_addr_,
                                server_port_,
                                0 )

    # Bytes of the struct, minus the uint32_t crc32 field
    flash_config_bytes = bytes(flash_config)[:-4]

    # Calculate the CRC32 of the struct
    flash_config.crc32 = calc_crc32(flash_config_bytes)

    # Bytes to write to flash
    provisioning_bytes = bytes(flash_config)

    # pad with FFs so it spans 2 pages
    provisioning_bytes += bytes( [0xFF] * (FLASH_CONFIG_DATA_END_ADDR - FLASH_CONFIG_DATA_START_ADDR - len(provisioning_bytes)) )

    os.makedirs(BIN_DIR, exist_ok=True)

    # Write to files
    filename_no_ext = os.path.join(BIN_DIR, f"a{anchor_id}")

    bin_filename = filename_no_ext + ".bin"
    hex_filename = filename_no_ext + ".hex"

    with open(bin_filename, "wb") as f_bin:
        f_bin.write( provisioning_bytes )

    bin2hex(bin_filename, hex_filename, FLASH_CONFIG_DATA_START_ADDR)

    print(f"Generated config images {hex_filename}")

def main():
    for i in range(5):
        generate_anchor_config(i)

if __name__ == '__main__':
    main()
