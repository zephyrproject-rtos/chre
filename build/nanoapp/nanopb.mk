#
# Nanoapp NanoPB Makefile
#
# Include this file in your nanoapp Makefile to produce pb.c and pb.h for .proto
# files specified in the NANOPB_SRCS variable. The produced pb.c files are
# automatically be added to the COMMON_SRCS variable for the nanoapp build.

include $(CHRE_PREFIX)/build/defs.mk

# Generated Source Files #######################################################

NANOPB_GEN_PATH = $(OUT)/nanopb_gen

NANOPB_GEN_SRCS += $(patsubst %.proto, $(NANOPB_GEN_PATH)/%.pb.c, \
                              $(NANOPB_SRCS))

# Generate NanoPB Sources ######################################################

COMMON_SRCS += $(NANOPB_GEN_SRCS)

NANOPB_PROJECT_PATH = $(ANDROID_BUILD_TOP)/external/nanopb-c
NANOPB_PROTOC = $(NANOPB_PROJECT_PATH)/generator/protoc-gen-nanopb

$(NANOPB_GEN_SRCS): $(NANOPB_GEN_PATH)/%.pb.c: %.proto $(wildcard %.options)
	mkdir -p $(dir $@)
	protoc --plugin=protoc-gen-nanopb=$(NANOPB_PROTOC) \
	  --nanopb_out=$(NANOPB_GEN_PATH) $<
