__THIS_DIR := $(patsubst %/,%,$(dir $(realpath $(lastword $(MAKEFILE_LIST)))))

include $(__THIS_DIR)/Vars.mk

.PHONY: pb
pb: $(PROTO_C_FILES) $(PROTO_H_FILES) $(PROTO_PY_FILES) | $(PROTO_BUILD_DIR) $(PB_C_BUILD_DIR) $(PB_PY_BUILD_DIR)

$(PB_C_BUILD_DIR)/%.pb.c $(PB_C_BUILD_DIR)/%.pb.h : %.proto | $(PROTO_BUILD_DIR) $(PB_C_BUILD_DIR) $(PB_PY_BUILD_DIR)
	$(PROTOC) $(PROTOC_INCLUDES) --nanopb_out=$(PB_C_BUILD_DIR) $<

$(PB_PY_BUILD_DIR)/%.py : %.proto | $(PROTO_BUILD_DIR) $(PB_C_BUILD_DIR) $(PB_PY_BUILD_DIR)
	$(PROTOC) $(PROTOC_INCLUDES) --python_out=$(PB_PY_BUILD_DIR) $<

$(PROTO_BUILD_DIR) $(PB_C_BUILD_DIR) $(PB_PY_BUILD_DIR):
	mkdir -p $@

.PHONY: clean_pb
clean_pb:
	rm -rf $(PROTO_BUILD_DIR)
	rm -rf $(PB_C_BUILD_DIR)
	rm -rf $(PB_PY_BUILD_DIR)
