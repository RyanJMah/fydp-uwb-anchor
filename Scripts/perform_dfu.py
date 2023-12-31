import sys
import time
import socket
import click
import intelhex
import paho.mqtt.client as mqtt
from math import ceil
from ctypes import LittleEndianStructure, c_uint32, c_uint8

from header_constants import (
    DFU_MSG_TYPE_REQ,
    DFU_MSG_TYPE_READY,
    DFU_MSG_TYPE_METADATA,
    DFU_MSG_TYPE_BEGIN,
    DFU_MSG_TYPE_CHUNK,
    DFU_MSG_TYPE_OK,
    DFU_MSG_TYPE_CONFIRM,
    DFU_MSG_TYPE_INVALID,
    DFU_MSG_IS_VALID,
    DFU_UPDATE_TYPE_APP_CODE,
    DFU_UPDATE_TYPE_CONFIG_DATA,
    DFU_CHUNK_SIZE,
    DFU_SERVER_PORT,
    DFU_TOPIC_FMT,
    FLASH_APP_SIZE,
    FLASH_PAGE_SIZE,
    DFU_HARDCODED_PASSWD
)
from Calc_CRC32 import calc_crc32


MAX_CHUNK_RETRIES = 10
CHUNK_RETRY_DELAY = 0 # seconds

# Sequence diagram:
#
#       Anchor                             Server
#         |                                  |
#  (running app code)                        |
#         |                                  |
#         |<------------- REQ ---------------|
#         |                                  |
# (running bootloader)                       |
#         |                                  |
#         |------------- READY ------------->|
#         |                                  |
#         |                                  |
#         |<----------- METADATA ------------|
#         |                                  |
#         |                                  |
#         |------------- BEGIN ------------->|
#         |                                  |
#         |                                  |
#         |<----------- CHUNK 0 -------------|
#         |                                  |
#         |-------------- OK --------------->|
#         |                                  |
#         |             ........             |
#         |                                  |
#         |<---------- CHUNK N-1 ------------|
#         |                                  |
#         |-------------- OK --------------->|
#         |                                  |
#         |                                  |
#         |------------ CONFIRM ------------>|
#         |                                  |
#  (running app code)                     (exit)

class PackedStruct(LittleEndianStructure):
    _pack_ = 1

# Refer to ./Common/inc/dfu_messages.h for the structs

class DFU_RequestMsg(PackedStruct):
    _fields_ = [
        ("msg_type",    c_uint8),
    ]

class DFU_ReadyMsg(PackedStruct):
    _fields_ = [
        ("msg_type",    c_uint8),
    ]

class DFU_MetadataMsg(PackedStruct):
    _fields_ = [
        ("msg_type",        c_uint8),
        ("img_crc",         c_uint32),
        ("img_num_chunks",  c_uint32),
        ("img_num_bytes",   c_uint32),
        ("update_type",     c_uint8),
    ]

class DFU_BeginMsg(PackedStruct):
    _fields_ = [
        ("msg_type",    c_uint8),
    ]

class DFU_ChunkMsg(PackedStruct):
    _fields_ = [
        ("msg_type",    c_uint8),
        ("chunk_num",   c_uint32),
        ("chunk_size",  c_uint32),
        ("chunk_data",  c_uint8 * DFU_CHUNK_SIZE),
        ("chunk_crc32", c_uint32)
    ]

class DFU_OkMsg(PackedStruct):
    _fields_ = [
        ("msg_type",    c_uint8),
        ("ok",          c_uint8),
    ]

class DFU_ConfirmMsg(PackedStruct):
    _fields_ = [
        ("msg_type",    c_uint8),
        ("ok",          c_uint8),
    ]

@click.command()
@click.option("--anchor", type=int, required=True, help="The ID of the anchor to perform DFU on")
@click.option("--img-path", type=click.Path(exists=True), required=True, help="The path to the image to be flashed")
@click.option("--config-update", type=bool, default=False, is_flag=True, help="Whether the update is for config data or app code (default: app code)")
@click.option("--skip-req", type=bool, is_flag=True, default=False, help="Whether to skip sending the REQ message")
@click.option("--broker-addr", type=str, default="localhost", help="The address of the MQTT broker")
@click.option("--broker-port", type=int, default=1883, help="The port of the MQTT broker")
def cli(anchor: int, img_path: str, config_update: bool, skip_req: bool, broker_addr: str, broker_port: int) -> None:
    # Get a binary blog of the image
    img_bytes: bytes

    if img_path.endswith(".hex"):
        with open(img_path, "r") as f:
            ih = intelhex.IntelHex()
            ih.loadhex(f)

            img_bytes = ih.tobinstr()

    elif img_path.endswith(".bin"):
        with open(img_path, "rb") as f:
            img_bytes = f.read()

    else:
        print("ERROR: image must be in .hex or .bin format")
        sys.exit(1)


    if len(img_bytes) > FLASH_APP_SIZE:
        print(f"ERROR: image is too large ({len(img_bytes)}, max size = {FLASH_APP_SIZE} bytes)")
        sys.exit(1)

    # pad with 0xFF to make the img completely fill the last page
    if (len(img_bytes) % FLASH_PAGE_SIZE) != 0:
        img_bytes += b"\xFF" * ( FLASH_PAGE_SIZE - (len(img_bytes) % FLASH_PAGE_SIZE) )

    if not skip_req:
        ############################################################################
        # REQ MESSAGE

        # The initial REQ message is sent to the anchor over MQTT, since
        # it would have been running the app code at this point.

        client = mqtt.Client()

        client.connect(broker_addr, broker_port)
        client.loop_start()

        client.publish( DFU_TOPIC_FMT % anchor, payload=DFU_HARDCODED_PASSWD.encode() )
        ############################################################################

    # The READY message is sent by the bootloader over TCP
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # This is so we can run the script again without waiting for the socket to timeout 
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    sock.bind( ("0.0.0.0", DFU_SERVER_PORT) )
    sock.listen()

    print("Waiting for anchor to connect...")
    conn, addr = sock.accept()
    print("Anchor connected!")

    if not skip_req:
        # We can stop the mqtt client now, it won't be needed anymore
        client.loop_stop()

    with conn:
        received: bytes

        ############################################################################
        # READY MESSAGE
        received = conn.recv( 1024 )

        ready_msg: DFU_ReadyMsg = DFU_ReadyMsg.from_buffer_copy(received)
        assert(ready_msg.msg_type == DFU_MSG_TYPE_READY)

        print("Received READY")
        ############################################################################

        ############################################################################
        # METADATA MESSAGE
        img_crc        = calc_crc32(img_bytes)
        img_num_chunks = ceil( len(img_bytes) / DFU_CHUNK_SIZE )

        update_type = DFU_UPDATE_TYPE_CONFIG_DATA if config_update else DFU_UPDATE_TYPE_APP_CODE

        metadata_msg = DFU_MetadataMsg( msg_type = DFU_MSG_TYPE_METADATA,
                                        img_crc = img_crc,
                                        img_num_chunks = img_num_chunks,
                                        img_num_bytes = len(img_bytes),
                                        update_type = update_type )
        conn.sendall( bytes(metadata_msg) )

        print( len(img_bytes))

        print(f"Sent METADATA: {img_num_chunks} chunks")
        ############################################################################

        ############################################################################
        # BEGIN MESSAGE
        received = conn.recv( 1024 )

        begin_msg: DFU_BeginMsg = DFU_BeginMsg.from_buffer_copy(received)
        assert(begin_msg.msg_type == DFU_MSG_TYPE_BEGIN)

        print("Received BEGIN")
        ############################################################################

        ############################################################################
        # CHUNK MESSAGES
        chunks = [ img_bytes[i:i+DFU_CHUNK_SIZE] for i in range(0, len(img_bytes), DFU_CHUNK_SIZE) ]

        def send_chunk(i: int) -> bool:
            chunk_size = len(chunks[i])
            uint8_arr  = (c_uint8 * chunk_size)(*chunks[i])

            chunk_msg = DFU_ChunkMsg( msg_type = DFU_MSG_TYPE_CHUNK,
                                      chunk_num = i,
                                      chunk_size = chunk_size,
                                      chunk_data = uint8_arr,
                                      chunk_crc32 = 0 )

            chunk_msg.chunk_crc32 = calc_crc32( bytes(chunks[i]) )

            conn.sendall( bytes(chunk_msg) )
            print(f"Sent CHUNK {i} / {len(chunks) - 1}, CRC = {chunk_msg.chunk_crc32:08X}")

            ###########################################################
            # OK MESSAGE
            received = conn.recv( 1024 )

            ok_msg: DFU_OkMsg = DFU_OkMsg.from_buffer_copy(received)
            assert(ok_msg.msg_type == DFU_MSG_TYPE_OK)
            ###########################################################

            print(f"Received OK: {bool(ok_msg.ok)}\n")

            return bool(ok_msg.ok)

        ok = False
        for i, chunk in enumerate(chunks):
            num_retries = 0

            while not ok:
                ok = send_chunk(i)  # Keep sending the chunk until we get an OK back                

                if num_retries > MAX_CHUNK_RETRIES:
                    print("Too many retries, aborting...")
                    sys.exit(1)

                num_retries += 1
                time.sleep(CHUNK_RETRY_DELAY)

            ok = False  # Reset the OK flag for the next chunk

        ############################################################################

        ############################################################################
        # CONFIRM MESSAGE
        received = conn.recv( 1024 )

        confirm_msg: DFU_ConfirmMsg = DFU_ConfirmMsg.from_buffer_copy(received)
        assert(confirm_msg.msg_type == DFU_MSG_TYPE_CONFIRM)

        if not confirm_msg.ok:
            print("Anchor failed to flash image!")
            sys.exit(1)
        ############################################################################

        print("Done")

if __name__ == "__main__":
    cli()
