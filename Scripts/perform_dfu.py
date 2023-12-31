import sys
import time
import socket
import click
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
    DFU_CHUNK_SIZE,
    DFU_SERVER_PORT,
    DFU_TOPIC_FMT
)
from Calc_CRC32 import calc_crc32

from server_code.server.mqtt_client import MqttClient
from server_code.server.gl_conf import GL_CONF

MAX_CHUNK_RETRIES = 10
CHUNK_RETRY_DELAY = 5 # seconds

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
        ("msg_type",            c_uint8),
        ("img_crc",             c_uint32),
        ("img_num_chunks",      c_uint32),
        ("img_num_bytes",       c_uint32),
        ("update_config_data",  c_uint8),
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
@click.option("--anchor-id", type=int, required=True, help="The ID of the anchor to perform DFU on")
@click.option("--img-path", type=click.Path(exists=True), required=True, help="The path to the image to be flashed")
@click.option("--update-config", type=bool, default=False, help="Whether to update the anchor config data in flash")
@click.option("--skip-req", type=bool, is_flag=True, default=False, help="Whether to skip sending the REQ message")
def cli(anchor_id: int, img_path: str, update_config: bool, skip_req: bool) -> None:
    # Get a binary blog of the image
    img_bytes: bytes

    with open(img_path, "rb") as f:
        img_bytes = f.read()

    if not skip_req:
        # The initial REQ message is sent to the anchor over MQTT, since
        # it would have been running the app code at this point.
        mqtt_client = MqttClient()

        mqtt_client.connect(GL_CONF.broker_address, GL_CONF.broker_port)
        mqtt_client.run_mainloop()

        ############################################################################
        # REQ MESSAGE
        req_msg = DFU_RequestMsg(msg_type = DFU_MSG_TYPE_REQ)
        mqtt_client.publish( DFU_TOPIC_FMT % anchor_id, bytes(req_msg) )
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
        mqtt_client.stop_mainloop()

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

        metadata_msg = DFU_MetadataMsg( msg_type = DFU_MSG_TYPE_METADATA,
                                        img_crc = img_crc,
                                        img_num_chunks = img_num_chunks,
                                        img_num_bytes = len(img_bytes),
                                        update_config_data = update_config )
        conn.sendall( bytes(metadata_msg) )

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
            print(f"Sent CHUNK {i}, CRC = {chunk_msg.chunk_crc32:08X}")
            print(f"chunk = {chunks[i]}")

            ###########################################################
            # OK MESSAGE
            received = conn.recv( 1024 )

            ok_msg: DFU_OkMsg = DFU_OkMsg.from_buffer_copy(received)
            assert(ok_msg.msg_type == DFU_MSG_TYPE_OK)
            ###########################################################

            print(f"Received OK: {bool(ok_msg.ok)}")

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

if __name__ == "__main__":
    cli()
