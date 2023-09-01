import socket
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
    DFU_CHUNK_SIZE
)

from server_code.server.mqtt_client import MqttClient
from server_code.server.gl_conf import GL_CONF

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
        ("msg_type",                    c_uint8),
        ("img_crc",                     c_uint32),
        ("img_num_chunks",              c_uint32),
        ("update_provisioning_data",    c_uint8),
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

def main() -> None:
    # The initial REQ message is sent to the anchor over MQTT, since
    # it would have been running the app code at this point.

    mqtt_client = MqttClient()

    mqtt_client.connect(GL_CONF.broker_address, GL_CONF.broker_port)
    mqtt_client.run_mainloop()

    REQ_msg = DFU_RequestMsg(msg_type = DFU_MSG_TYPE_REQ)
    mqtt_client.publish(, REQ_msg)

    # The READY message is sent by the bootloader over TCP
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # This is so we can restart the program without waiting for the socket to timeout 
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    sock.bind( ("0.0.0.0", DFU_SERVER_PORT) )
    sock.listen()

    print("Waiting for anchor to connect...")
    conn, addr = sock.accept()
    print("Anchor connected!")

    # We can stop the mqtt client now, it won't be needed anymore
    mqtt_client.loop_stop()

    with conn:
        # The anchor will send the READY message
        msg: bytes = conn.recv( 1024 )

        ready_msg = DFU_ReadyMsg.from_buffer_copy(ready_msg)



if __name__ == "__main__":
    main()
