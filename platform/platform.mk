#
# Platform Makefile
#

include $(CHRE_PREFIX)/external/flatbuffers/flatbuffers.mk

# Common Compiler Flags ########################################################

# Include paths.
COMMON_CFLAGS += -Iplatform/include

# Common Compiler Flags ########################################################

# SLPI-specific Compiler Flags #################################################

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

SLPI_CFLAGS += -Iplatform/shared/include
SLPI_CFLAGS += -Iplatform/slpi/include

# We use FlatBuffers in the SLPI platform layer
SLPI_CFLAGS += $(FLATBUFFERS_CFLAGS)

# SLPI/SMGR-specific Compiler Flags ############################################

# Include paths.
SLPI_SMGR_CFLAGS += -I$(SLPI_PREFIX)/Sensors/api
SLPI_SMGR_CFLAGS += -I$(SLPI_PREFIX)/Sensors/common/idl/inc
SLPI_SMGR_CFLAGS += -I$(SLPI_PREFIX)/Sensors/common/inc
SLPI_SMGR_CFLAGS += -I$(SLPI_PREFIX)/Sensors/common/smr/inc
SLPI_SMGR_CFLAGS += -I$(SLPI_PREFIX)/Sensors/common/util/mathtools/inc
SLPI_SMGR_CFLAGS += -I$(SLPI_PREFIX)/Sensors/goog/api
SLPI_SMGR_CFLAGS += -I$(SLPI_PREFIX)/Sensors/pm/inc

SLPI_SMGR_CFLAGS += -Iplatform/slpi/smgr/include

SLPI_SMGR_CFLAGS += -DCHRE_SLPI_SMGR

# SLPI/SEE-specific Compiler Flags #############################################

# Include paths.
SLPI_SEE_CFLAGS += -I$(SLPI_PREFIX)/qmimsgs/common/api
SLPI_SEE_CFLAGS += -I$(SLPI_PREFIX)/ssc/framework/cm/inc
SLPI_SEE_CFLAGS += -I$(SLPI_PREFIX)/ssc/goog/api

SLPI_SEE_CFLAGS += -Iplatform/slpi/see/include

SLPI_SEE_CFLAGS += -DCHRE_SLPI_SEE

# SLPI-specific Source Files ###################################################

SLPI_SRCS += platform/shared/chre_api_audio.cc
SLPI_SRCS += platform/shared/chre_api_core.cc
SLPI_SRCS += platform/shared/chre_api_gnss.cc
SLPI_SRCS += platform/shared/chre_api_re.cc
SLPI_SRCS += platform/shared/chre_api_sensor.cc
SLPI_SRCS += platform/shared/chre_api_version.cc
SLPI_SRCS += platform/shared/chre_api_wifi.cc
SLPI_SRCS += platform/shared/chre_api_wwan.cc
SLPI_SRCS += platform/shared/host_protocol_chre.cc
SLPI_SRCS += platform/shared/host_protocol_common.cc
SLPI_SRCS += platform/shared/memory_manager.cc
SLPI_SRCS += platform/shared/nanoapp/nanoapp_dso_util.cc
SLPI_SRCS += platform/shared/pal_system_api.cc
SLPI_SRCS += platform/shared/platform_gnss.cc
SLPI_SRCS += platform/shared/platform_wifi.cc
SLPI_SRCS += platform/shared/platform_wwan.cc
SLPI_SRCS += platform/shared/system_time.cc
SLPI_SRCS += platform/slpi/chre_api_re.cc
SLPI_SRCS += platform/slpi/fatal_error.cc
SLPI_SRCS += platform/slpi/host_link.cc
SLPI_SRCS += platform/slpi/init.cc
SLPI_SRCS += platform/slpi/memory.cc
SLPI_SRCS += platform/slpi/memory_manager.cc
SLPI_SRCS += platform/slpi/platform_log.cc
SLPI_SRCS += platform/slpi/platform_nanoapp.cc
SLPI_SRCS += platform/slpi/platform_pal.cc
SLPI_SRCS += platform/slpi/preloaded_nanoapps.cc
SLPI_SRCS += platform/slpi/system_time.cc
SLPI_SRCS += platform/slpi/system_timer.cc

# Optional audio support.
ifneq ($(CHRE_AUDIO_SUPPORT_ENABLED),)
SLPI_SRCS += platform/slpi/platform_audio.cc
endif

# SLPI/SMGR-specific Source Files ##############################################

SLPI_SMGR_SRCS += platform/slpi/smgr/platform_sensor.cc
SLPI_SMGR_SRCS += platform/slpi/smgr/platform_sensor_util.cc
SLPI_SMGR_SRCS += platform/slpi/smgr/power_control_manager.cc
SLPI_SMGR_SRCS += platform/slpi/smgr/smr_helper.cc

# SLPI/SEE-specific Source Files ###############################################

SLPI_SEE_SRCS += platform/slpi/see/platform_sensor.cc
SLPI_SEE_SRCS += platform/slpi/see/power_control_manager.cc

# Simulator-specific Compiler Flags ############################################

SIM_CFLAGS += -Iplatform/shared/include

# Simulator-specific Source Files ##############################################

SIM_SRCS += platform/linux/chre_api_re.cc
SIM_SRCS += platform/linux/context.cc
SIM_SRCS += platform/linux/fatal_error.cc
SIM_SRCS += platform/linux/host_link.cc
SIM_SRCS += platform/linux/memory.cc
SIM_SRCS += platform/linux/memory_manager.cc
SIM_SRCS += platform/linux/platform_log.cc
SIM_SRCS += platform/linux/platform_pal.cc
SIM_SRCS += platform/linux/power_control_manager.cc
SIM_SRCS += platform/linux/system_time.cc
SIM_SRCS += platform/linux/system_timer.cc
SIM_SRCS += platform/linux/platform_nanoapp.cc
SIM_SRCS += platform/linux/platform_sensor.cc
SIM_SRCS += platform/shared/chre_api_audio.cc
SIM_SRCS += platform/shared/chre_api_core.cc
SIM_SRCS += platform/shared/chre_api_gnss.cc
SIM_SRCS += platform/shared/chre_api_re.cc
SIM_SRCS += platform/shared/chre_api_sensor.cc
SIM_SRCS += platform/shared/chre_api_version.cc
SIM_SRCS += platform/shared/chre_api_wifi.cc
SIM_SRCS += platform/shared/chre_api_wwan.cc
SIM_SRCS += platform/shared/memory_manager.cc
SIM_SRCS += platform/shared/nanoapp/nanoapp_dso_util.cc
SIM_SRCS += platform/shared/pal_gnss_stub.cc
SIM_SRCS += platform/shared/pal_wifi_stub.cc
SIM_SRCS += platform/shared/pal_wwan_stub.cc
SIM_SRCS += platform/shared/pal_system_api.cc
SIM_SRCS += platform/shared/platform_gnss.cc
SIM_SRCS += platform/shared/platform_wifi.cc
SIM_SRCS += platform/shared/platform_wwan.cc
SIM_SRCS += platform/shared/system_time.cc

# Linux-specific Compiler Flags ################################################

GOOGLE_X86_LINUX_CFLAGS += -Iplatform/linux/include

# Linux-specific Source Files ##################################################

GOOGLE_X86_LINUX_SRCS += platform/linux/audio_source.cc
GOOGLE_X86_LINUX_SRCS += platform/linux/platform_audio.cc
GOOGLE_X86_LINUX_SRCS += platform/linux/init.cc

# Android-specific Source Files ################################################

# Add the Android include search path for Android-specific header files.
GOOGLE_ARM64_ANDROID_CFLAGS += -Iplatform/android/include

# Also add the linux sources to fall back to the default Linux implementation.
GOOGLE_ARM64_ANDROID_CFLAGS += -Iplatform/linux/include

# Android-specific Source Files ################################################

GOOGLE_ARM64_ANDROID_SRCS += platform/android/init.cc

# GoogleTest Compiler Flags ####################################################

GOOGLETEST_CFLAGS += -Iplatform/slpi/include

# GoogleTest Source Files ######################################################

GOOGLETEST_SRCS += platform/linux/assert.cc
GOOGLETEST_SRCS += platform/slpi/smgr/platform_sensor_util.cc
GOOGLETEST_SRCS += platform/slpi/smgr/tests/platform_sensor_util_test.cc
