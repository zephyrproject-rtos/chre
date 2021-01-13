#
# CHRE GNSS Concurrent Cross Validation Test Nanoapp Makefile
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

NANOAPP_NAME = chre_gnss_concurrent_cross_validation
NANOAPP_ID = 0x476f6f6754000006
NANOAPP_NAME_STRING = \"CHRE\ GNSS\ Concurrent\ Cross\ Validation\ Test\"
NANOAPP_VERSION = 0x00000001

NANOAPP_PATH = $(CHRE_PREFIX)/apps/test/common/chre_gnss_concurrent_cross_validation

# Source Code ##################################################################

COMMON_SRCS += $(NANOAPP_PATH)/src/chre_gnss_concurrent_cross_validation.cc

# Compiler Flags ###############################################################

# Defines
COMMON_CFLAGS += -DNANOAPP_MINIMUM_LOG_LEVEL=CHRE_LOG_LEVEL_INFO

# Permission declarations ######################################################

CHRE_NANOAPP_USES_GNSS = true

# Makefile Includes ############################################################

include $(CHRE_PREFIX)/build/nanoapp/app.mk
