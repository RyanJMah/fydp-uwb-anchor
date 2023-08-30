import socket

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
#         |<----------- METADATA ------------|
#         |                                  |
#         |------------- BEGIN ------------->|
#         |                                  |
#         |<----------- CHUNK 0 -------------|
#         |                                  |
#         |             ........             |
#         |                                  |
#         |<---------- CHUNK N-1 ------------|
#         |                                  |
#         |------------ CONFIRM ------------>|
#         |                                  |
#  (running app code)                     (exit)

def main():
    pass

if __name__ == "__main__":
    main()
