#
# CHRE Makefile
#

# Environment Setup ############################################################

# Building CHRE is always done in-tree so the CHRE_PREFIX can be assigned to the
# current directory.
CHRE_PREFIX = .

# Environment Checks ###########################################################

# Ensure that the user has specified a path to the Hexagon SDK and SLPI tree
# which are required build the runtime.
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

# Build Configuration ##########################################################

OUTPUT_NAME = libchre

# Include Paths ################################################################

# Hexagon Include Paths
HEXAGON_CFLAGS += -I$(HEXAGON_SDK_PREFIX)/incs
HEXAGON_CFLAGS += -I$(HEXAGON_SDK_PREFIX)/incs/stddef
HEXAGON_CFLAGS += -I$(SLPI_PREFIX)/build/ms
HEXAGON_CFLAGS += -I$(SLPI_PREFIX)/build/cust
HEXAGON_CFLAGS += -I$(SLPI_PREFIX)/core/api/services
HEXAGON_CFLAGS += -I$(SLPI_PREFIX)/core/api/kernel/devcfg
HEXAGON_CFLAGS += -I$(SLPI_PREFIX)/core/api/kernel/qurt
HEXAGON_CFLAGS += -I$(SLPI_PREFIX)/core/api/dal
HEXAGON_CFLAGS += -I$(SLPI_PREFIX)/core/api/mproc
HEXAGON_CFLAGS += -I$(SLPI_PREFIX)/core/api/systemdrivers
HEXAGON_CFLAGS += -I$(SLPI_PREFIX)/Sensors/api
HEXAGON_CFLAGS += -I$(SLPI_PREFIX)/Sensors/common/util/mathtools/inc

# Compiler Flags ###############################################################

# Define CUST_H to allow including the customer header file.
HEXAGON_CFLAGS += -DCUST_H

# Makefile Includes ############################################################

include apps/hello_world/hello_world.mk
include apps/sensor_world/sensor_world.mk
include apps/timer_world/timer_world.mk
include chre_api/chre_api.mk
include core/core.mk
include pal/pal.mk
include platform/platform.mk
include util/util.mk

# Common Includes
include build/common.mk

# Supported Variants Includes. Not all CHRE variants are supported by this
# implementation of CHRE. Example: this CHRE implementation is never built for
# google_cm4_nanohub as Nanohub itself is a CHRE implementation.
include $(CHRE_PREFIX)/build/variant/google_hexagon_v60_slpi.mk
include $(CHRE_PREFIX)/build/variant/google_hexagon_v62_slpi.mk
