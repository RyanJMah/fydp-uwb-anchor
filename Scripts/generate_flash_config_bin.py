import os
from ctypes import LittleEndianStructure, c_uint32, c_uint8, c_char
from intelhex import bin2hex

THIS_DIR      = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT_DIR = os.path.dirname(THIS_DIR)
BIN_DIR       = os.path.join(REPO_ROOT_DIR, "Provisioning_Images")

FLASH_CONFIG_START_ADDR = 0x00005000    # Refer to ./Common/flash_memory_map.h
NUM_FALLBACK_SERVERS    = 10
MAX_HOSTNAME_CHARS      = 256

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
        ("c", c_char * 128)
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
    ]

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

def main():
    #########################################################################
    swap_count = 0

    fw_update_pending = 0

    anchor_id = 2

    socket_recv_timeout_ms = 5000

    mac_addr = [0x00, 0x00, 0x00, 0x00, 0x00, 0x00]
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

    mac_addr_       = (c_uint8 * 6)(*mac_addr)

    static_ip_addr_ = Ipv4_Addr.from_list(static_ip_addr)
    static_netmask_ = Ipv4_Addr.from_list(static_netmask)
    static_gateway_ = Ipv4_Addr.from_list(static_gateway)

    server_hostname_ = [ Hostname.from_bytes(b) for b in server_hostname ]
    server_hostname_ = (Hostname * NUM_FALLBACK_SERVERS)(*server_hostname_)

    server_ip_addr_ = [ Ipv4_Addr.from_list(a) for a in server_ip_addr ]
    server_ip_addr_ = (Ipv4_Addr * NUM_FALLBACK_SERVERS)(*server_ip_addr_)

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
                                server_port_ )

    filename_no_ext = os.path.join(BIN_DIR, f"a{anchor_id}")

    bin_filename = filename_no_ext + ".bin"
    hex_filename = filename_no_ext + ".hex"

    with open(bin_filename, "wb") as f_bin:
        f_bin.write( bytes(flash_config) )

    bin2hex(bin_filename, hex_filename, FLASH_CONFIG_START_ADDR)

if __name__ == '__main__':
    main()
