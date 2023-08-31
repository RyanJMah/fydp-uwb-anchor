__THIS_DIR := $(patsubst %/,%,$(dir $(realpath $(lastword $(MAKEFILE_LIST)))))

NANOPB_DIR := $(__THIS_DIR)/../nanopb
PROTOC     := $(NANOPB_DIR)/generator/protoc

# Directory where .proto files are located
PROTO_SRC_DIR := $(__THIS_DIR)

# Directory where .pb.h and .pb.c files will be generated
PROTO_BUILD_DIR := $(PROTO_SRC_DIR)/AutoGenerated

PB_C_BUILD_DIR  := $(PROTO_BUILD_DIR)/C
PB_PY_BUILD_DIR := $(PROTO_BUILD_DIR)/Py

# List of .proto files
PROTO_SRC_FILES := $(wildcard $(PROTO_SRC_DIR)/*.proto)

# Generated C sources and headers from .proto files
PROTO_C_FILES := $(patsubst $(PROTO_SRC_DIR)/%.proto, $(PB_C_BUILD_DIR)/%.pb.c, $(PROTO_SRC_FILES))
PROTO_H_FILES := $(patsubst $(PROTO_SRC_DIR)/%.proto, $(PB_C_BUILD_DIR)/%.pb.h, $(PROTO_SRC_FILES))

NANOPB_SRC_FILES += $(NANOPB_DIR)/pb_encode.c
NANOPB_SRC_FILES += $(NANOPB_DIR)/pb_decode.c
NANOPB_SRC_FILES += $(NANOPB_DIR)/pb_common.c

NANOPB_INCLUDES := $(NANOPB_DIR)
