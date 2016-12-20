#
# Build CHRE into a static archive to be linked by the toolchain.
#

include build/defs.mk

.PHONY: hexagon

# Hexagon Check ################################################################

# If building for the Hexagon target, ensure that the user has specified a path
# to the Hexagon toolchain that they wish to use.

ifneq ($(filter hexagon all, $(MAKECMDGOALS)),)
ifeq ($(HEXAGON_TOOLS_PREFIX),)
$(error "You must supply a HEXAGON_TOOLS_PREFIX environment variable \
         containing a path to the hexagon toolchain. Example: \
         $$HOME/Qualcomm/HEXAGON_Tools/8.0.07")
endif

ifeq ($(HEXAGON_SDK_PREFIX),)
$(error "You must supply a HEXAGON_SDK_PREFIX environment variable \
         containing a path to the hexagon SDK. Example: \
         $$HOME/Qualcomm/Hexagon_SDK/3.0")
endif

ifeq ($(SLPI_PREFIX),)
$(error "You must supply an SLPI_PREFIX environemnt variable \
         containing a path to the SLPI source tree. Example: \
         $$HOME/slpi_proc")
endif
endif

# Hexagon Tools ################################################################

HEXAGON_CC = $(HEXAGON_TOOLS_PREFIX)/Tools/bin/hexagon-clang
HEXAGON_AR = $(HEXAGON_TOOLS_PREFIX)/Tools/bin/hexagon-ar

# Hexagon Compiler Flags #######################################################

HEXAGON_CFLAGS += $(COMMON_CFLAGS)

# Include paths.
HEXAGON_CFLAGS += -I$(HEXAGON_SDK_PREFIX)/incs
HEXAGON_CFLAGS += -I$(HEXAGON_SDK_PREFIX)/incs/stddef
HEXAGON_CFLAGS += -I$(SLPI_PREFIX)/build/ms
HEXAGON_CFLAGS += -I$(SLPI_PREFIX)/build/cust
HEXAGON_CFLAGS += -I$(SLPI_PREFIX)/core/api/services
HEXAGON_CFLAGS += -I$(SLPI_PREFIX)/core/api/kernel/devcfg
HEXAGON_CFLAGS += -I$(SLPI_PREFIX)/core/api/dal
HEXAGON_CFLAGS += -I$(SLPI_PREFIX)/core/api/systemdrivers

# Define CUST_H to allow including the customer header file.
HEXAGON_CFLAGS += -DCUST_H

# Enable position independence.
HEXAGON_CFLAGS += -fpic

# Enable hexagon v6.2 architecture.
HEXAGON_CFLAGS += -mv62

# Disable splitting double registers.
HEXAGON_CFLAGS += -mllvm -disable-hsdr

# Enable the POSIX threading model.
HEXAGON_CFLAGS += -mthread-model posix

# This code is loaded into a dynamic module. Define this symbol in the event
# that any Qualcomm code needs it.
HEXAGON_CFLAGS += -D__V_DYNAMIC__

# TODO: Consider options for setting the optimization level. Maybe an
# environment variable can suffice here.
HEXAGON_CFLAGS += -O0

# TODO: Probably want to disable this when compiling for >-O0.
HEXAGON_CFLAGS += -D_DEBUG

# Hexagon Static Archive #######################################################

HEXAGON_ARFLAGS = -rsc

HEXAGON_ARCHIVE = $(OUT)/hexagon/libchre.a

# Hexagon Objects ##############################################################

HEXAGON_SRCS += $(COMMON_SRCS)
HEXAGON_OBJS = $(patsubst %.cc, $(OUT)/hexagon/%.o, $(HEXAGON_SRCS))
HEXAGON_DIRS = $(sort $(dir $(HEXAGON_OBJS)))

# Top-level Hexagon Build Rule #################################################

hexagon: $(OUT) $(HEXAGON_DIRS) $(HEXAGON_ARCHIVE)

all: hexagon

# Output Directories ###########################################################

$(HEXAGON_DIRS):
	mkdir -p $@

# Link #########################################################################

$(HEXAGON_ARCHIVE): $(HEXAGON_OBJS)
	$(HEXAGON_AR) $(HEXAGON_ARFLAGS) $@ $^

# Compile ######################################################################

$(HEXAGON_OBJS): $(OUT)/hexagon/%.o: %.cc
	$(HEXAGON_CC) $(HEXAGON_CFLAGS) -c $< -o $@
