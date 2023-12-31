import zlib

# def calc_crc32(data: bytes) -> int:
#     crc = 0xFFFFFFFF
#     for byte in data:
#         crc ^= byte

#         for _ in range(8):
#             if crc & 1:
#                 crc = (crc >> 1) ^ 0xEDB88320
#             else:
#                 crc = (crc >> 1) ^ 0

#     return crc ^ 0xFFFFFFFF

def calc_crc32(data: bytes) -> int:
    return zlib.crc32(data) & 0xFFFFFFFF