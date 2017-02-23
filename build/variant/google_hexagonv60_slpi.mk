#
# Google CHRE Reference Implementation for Hexagon v60 Architecture on SLPI
#

include $(CHRE_PREFIX)/build/clean_build_template_args.mk

TARGET_NAME = google_hexagonv60_slpi
TARGET_CFLAGS = -DCHRE_MESSAGE_TO_HOST_MAX_SIZE=2048
HEXAGON_ARCH = v60

ifneq ($(filter $(TARGET_NAME)% all, $(MAKECMDGOALS)),)
include $(CHRE_PREFIX)/build/arch/hexagon.mk
include $(CHRE_PREFIX)/build/build_template.mk

ifneq ($(IS_NANOAPP_BUILD),)
include $(CHRE_PREFIX)/build/nanoapp/google_slpi.mk
endif
endif
