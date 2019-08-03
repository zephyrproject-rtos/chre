#
# Power Test Nanoapp Makefile
#

# Environment Checks ###########################################################

ifeq ($(CHRE_PREFIX),)
ifneq ($(ANDROID_BUILD_TOP),)
CHRE_PREFIX = $(ANDROID_BUILD_TOP)/system/chre
else
$(error "You must run 'lunch' to setup ANDROID_BUILD_TOP, or explicitly define \
         the CHRE_PREFIX environment variable to point to the CHRE root \
         directory.")
endif
endif

# Nanoapp Configuration ########################################################

# The same nanoapp can be built for TCM or not so the app details should change
# depending on the mode it's built for. -DCHRE_SLPI_UIMG_ENABLED is defined
# whenever CHRE is built for TCM on SLPI so use that to determine which mode
# the nanoapp is being built for.
ifneq ($(filter $(-DCHRE_SLPI_UIMG_ENABLED), $(TARGET_CFLAGS)), )
NANOAPP_NAME = power_test_tcm
NANOAPP_ID = 0x0123456789000010

NANOAPP_NAME_STRING = \"Power\ Test\ TCM\"
COMMON_CFLAGS += -DCHRE_TCM_BUILD
else
NANOAPP_NAME = power_test
NANOAPP_ID = 0x012345678900000f

NANOAPP_NAME_STRING = \"Power\ Test\"
endif

NANOAPP_VERSION = 0x00000001

# Common Compiler Flags ########################################################

# Include paths.
COMMON_CFLAGS += -I./include
COMMON_CFLAGS += -I$(CHRE_PREFIX)/external/flatbuffers/include
COMMON_CFLAGS += -I$(CHRE_PREFIX)/util/include

# Defines
COMMON_CFLAGS += -DNANOAPP_MINIMUM_LOG_LEVEL=CHRE_LOG_LEVEL_INFO

# Common Source Files ##########################################################

COMMON_SRCS += power_test.cc
COMMON_SRCS += request_manager.cc
COMMON_SRCS += $(CHRE_PREFIX)/util/dynamic_vector_base.cc
COMMON_SRCS += $(CHRE_PREFIX)/util/nanoapp/audio.cc
COMMON_SRCS += $(CHRE_PREFIX)/util/nanoapp/callbacks.cc

# Makefile Includes ############################################################

include $(CHRE_PREFIX)/build/nanoapp/app.mk
include $(CHRE_PREFIX)/external/external.mk
