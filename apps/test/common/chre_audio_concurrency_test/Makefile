#
# CHRE Audio Concurrency Test Nanoapp Makefile
#

# Environment Checks ###########################################################
ifeq ($(CHRE_PREFIX),)
  ifneq ($(ANDROID_BUILD_TOP),)
    CHRE_PREFIX = $(ANDROID_BUILD_TOP)/system/chre
  else
    $(error "You must run 'lunch' to setup ANDROID_BUILD_TOP, or explicitly \
    define the CHRE_PREFIX environment variable to point to the CHRE root \
    directory.")
  endif
endif

# Nanoapp Configuration ########################################################

NANOAPP_NAME = chre_audio_concurrency_test
NANOAPP_ID = 0x476f6f6754000004
NANOAPP_NAME_STRING = \"Chre\ Audio\ Concurrency\ Test\"
NANOAPP_VERSION = 0x00000001

NANOAPP_PATH = $(CHRE_PREFIX)/apps/test/common/chre_audio_concurrency_test

# Source Code ##################################################################

COMMON_SRCS += $(NANOAPP_PATH)/src/chre_audio_concurrency_test.cc

# Compiler Flags ###############################################################

# Defines
COMMON_CFLAGS += -DNANOAPP_MINIMUM_LOG_LEVEL=CHRE_LOG_LEVEL_INFO

# Includes
COMMON_CFLAGS += -I$(NANOAPP_PATH)/inc

# Makefile Includes ############################################################

include $(CHRE_PREFIX)/build/nanoapp/app.mk
