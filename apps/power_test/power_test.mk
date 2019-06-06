#
# Power Test Makefile
#

FLAT_BUFFERS_PREFIX = $(CHRE_PREFIX)/external/flatbuffers

# Common Compiler Flags ########################################################

# Include paths.
COMMON_CFLAGS += -Iapps/power_test/include
COMMON_CFLAGS += -I$(FLAT_BUFFERS_PREFIX)/include

# Common Source Files ##########################################################

COMMON_SRCS += apps/power_test/power_test.cc
COMMON_SRCS += apps/power_test/request_manager.cc
