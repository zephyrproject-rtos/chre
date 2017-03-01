#
# CHRE GoogleTest Build Variant
#

include $(CHRE_PREFIX)/build/clean_build_template_args.mk

TARGET_NAME = google_x86_googletest
TARGET_CFLAGS = -DCHRE_MESSAGE_TO_HOST_MAX_SIZE=2048
TARGET_CFLAGS += $(GOOGLE_X86_GOOGLETEST_CFLAGS)
TARGET_VARIANT_SRCS = $(GOOGLE_X86_GOOGLETEST_SRCS)

ifneq ($(filter $(TARGET_NAME)% all, $(MAKECMDGOALS)),)
ifeq ($(GOOGLETEST_PREFIX),)
$(error "You must supply a GOOGLETEST_PREFIX environment variable \
         containing a path to the Google Test project tree. Example \
         export GOOGLETEST_PREFIX=$$HOME/android/master/external/googletest")
endif

# Instruct the build to link a final executable.
TARGET_BUILD_BIN = true

# Link in libraries for the final executable.
TARGET_BIN_LDFLAGS += -lrt
TARGET_BIN_LDFLAGS += -lpthread

include $(CHRE_PREFIX)/build/arch/x86.mk
include $(CHRE_PREFIX)/build/build_template.mk
endif
