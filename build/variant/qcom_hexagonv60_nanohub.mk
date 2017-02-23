#
# Qualcomm CHRE Implementation for Hexagon v60, based on Nanohub
#

include $(CHRE_PREFIX)/build/clean_build_template_args.mk

TARGET_NAME = qcom_hexagonv60_nanohub
TARGET_CFLAGS = -DCHRE_MESSAGE_TO_HOST_MAX_SIZE=4080
TARGET_VARIANT_SRCS = $(QCOM_HEXAGONV60_NANOHUB_SRCS)
HEXAGON_ARCH = v60

ifneq ($(filter $(TARGET_NAME)% all, $(MAKECMDGOALS)),)
include $(CHRE_PREFIX)/build/arch/hexagon.mk
include $(CHRE_PREFIX)/build/build_template.mk

ifneq ($(IS_NANOAPP_BUILD),)
# TODO: Add support for building a nanoapp that can be loaded via the HAL.
endif
endif
