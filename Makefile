#
# CHRE Makefile
#

# Environment Setup ############################################################

# Building CHRE is always done in-tree so the CHRE_PREFIX can be assigned to the
# current directory.
CHRE_PREFIX = .

# Build Configuration ##########################################################

OUTPUT_NAME = libchre

# Compiler Flags ###############################################################

# Symbols required by the runtime for conditional compilation.
COMMON_CFLAGS += -DCHRE_MINIMUM_LOG_LEVEL=CHRE_LOG_LEVEL_DEBUG
COMMON_CFLAGS += -DNANOAPP_MINIMUM_LOG_LEVEL=CHRE_LOG_LEVEL_DEBUG
COMMON_CFLAGS += -DCHRE_ASSERTIONS_ENABLED

# Place nanoapps in a namespace.
COMMON_CFLAGS += -DCHRE_NANOAPP_INTERNAL

ifeq ($(CHRE_PATCH_VERSION),)
# Compute the patch version as the number of hours since the start of some
# arbitrary epoch. This will roll over 16 bits after ~7 years, but patch version
# is scoped to the API version, so we can adjust the offset when a new API
# version is released.
EPOCH=$(shell date --date='2017-01-01' +%s)
CHRE_PATCH_VERSION = $(shell echo $$(((`date +%s` - $(EPOCH)) / (60 * 60))))
endif

COMMON_CFLAGS += -DCHRE_PATCH_VERSION=$(CHRE_PATCH_VERSION)

# SLPI Flags ###################################################################

# Define CUST_H to allow including the customer header file.
SLPI_CFLAGS += -DCUST_H

# Include paths.
SLPI_CFLAGS += -I$(SLPI_PREFIX)/build/ms
SLPI_CFLAGS += -I$(SLPI_PREFIX)/build/cust
SLPI_CFLAGS += -I$(SLPI_PREFIX)/core/api/debugtools
SLPI_CFLAGS += -I$(SLPI_PREFIX)/core/api/services
SLPI_CFLAGS += -I$(SLPI_PREFIX)/core/api/kernel/devcfg
SLPI_CFLAGS += -I$(SLPI_PREFIX)/core/api/kernel/qurt
SLPI_CFLAGS += -I$(SLPI_PREFIX)/core/api/dal
SLPI_CFLAGS += -I$(SLPI_PREFIX)/core/api/mproc
SLPI_CFLAGS += -I$(SLPI_PREFIX)/core/api/systemdrivers
SLPI_CFLAGS += -I$(SLPI_PREFIX)/platform/inc
SLPI_CFLAGS += -I$(SLPI_PREFIX)/platform/inc/HAP
SLPI_CFLAGS += -I$(SLPI_PREFIX)/platform/inc/stddef
SLPI_CFLAGS += -I$(SLPI_PREFIX)/platform/rtld/inc
SLPI_CFLAGS += -I$(SLPI_PREFIX)/Sensors/api
SLPI_CFLAGS += -I$(SLPI_PREFIX)/Sensors/common/idl/inc
SLPI_CFLAGS += -I$(SLPI_PREFIX)/Sensors/common/inc
SLPI_CFLAGS += -I$(SLPI_PREFIX)/Sensors/common/smr/inc
SLPI_CFLAGS += -I$(SLPI_PREFIX)/Sensors/common/util/mathtools/inc
SLPI_CFLAGS += -I$(SLPI_PREFIX)/Sensors/goog/api
SLPI_CFLAGS += -I$(SLPI_PREFIX)/Sensors/pm/inc

# Makefile Includes ############################################################

# Variant-specific includes.
include $(CHRE_VARIANT_MK_INCLUDES)

# CHRE Implementation includes.
include apps/apps.mk
include ash/ash.mk
include chre_api/chre_api.mk
include core/core.mk
include external/external.mk
include pal/pal.mk
include platform/platform.mk
include util/util.mk

# Common includes.
include build/common.mk

# Supported Variants Includes. Not all CHRE variants are supported by this
# implementation of CHRE. Example: this CHRE implementation is never built for
# google_cm4_nanohub as Nanohub itself is a CHRE implementation.
include $(CHRE_PREFIX)/build/variant/google_hexagonv60_slpi.mk
include $(CHRE_PREFIX)/build/variant/google_hexagonv62_slpi.mk
include $(CHRE_PREFIX)/build/variant/google_hexagonv62_slpi-uimg.mk
include $(CHRE_PREFIX)/build/variant/google_x86_linux.mk
include $(CHRE_PREFIX)/build/variant/google_x86_googletest.mk
