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
SLPI_CFLAGS += -Iplatform/slpi/smgr/include

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

# SLPI-specific Source Files ###################################################

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
SLPI_SRCS += platform/slpi/power_control_manager.cc
SLPI_SRCS += platform/slpi/preloaded_nanoapps.cc
SLPI_SRCS += platform/slpi/system_time.cc
SLPI_SRCS += platform/slpi/system_timer.cc

# SLPI/SMGR-specific Source Files ###########################################

SLPI_SMGR_SRCS += platform/slpi/smgr/platform_sensor.cc
SLPI_SMGR_SRCS += platform/slpi/smgr/platform_sensor_util.cc
SLPI_SMGR_SRCS += platform/slpi/smgr/smr_helper.cc

# x86-specific Compiler Flags ##################################################

X86_CFLAGS += -Iplatform/shared/include
X86_CFLAGS += -Iplatform/linux/include

# x86-specific Source Files ####################################################

X86_SRCS += platform/linux/chre_api_re.cc
X86_SRCS += platform/linux/context.cc
X86_SRCS += platform/linux/fatal_error.cc
X86_SRCS += platform/linux/host_link.cc
X86_SRCS += platform/linux/memory.cc
X86_SRCS += platform/linux/memory_manager.cc
X86_SRCS += platform/linux/platform_log.cc
X86_SRCS += platform/linux/platform_pal.cc
X86_SRCS += platform/linux/power_control_manager.cc
X86_SRCS += platform/linux/system_time.cc
X86_SRCS += platform/linux/system_timer.cc
X86_SRCS += platform/linux/platform_nanoapp.cc
X86_SRCS += platform/linux/platform_sensor.cc
X86_SRCS += platform/shared/chre_api_core.cc
X86_SRCS += platform/shared/chre_api_gnss.cc
X86_SRCS += platform/shared/chre_api_re.cc
X86_SRCS += platform/shared/chre_api_sensor.cc
X86_SRCS += platform/shared/chre_api_version.cc
X86_SRCS += platform/shared/chre_api_wifi.cc
X86_SRCS += platform/shared/chre_api_wwan.cc
X86_SRCS += platform/shared/memory_manager.cc
X86_SRCS += platform/shared/nanoapp/nanoapp_dso_util.cc
X86_SRCS += platform/shared/pal_gnss_stub.cc
X86_SRCS += platform/shared/pal_wifi_stub.cc
X86_SRCS += platform/shared/pal_wwan_stub.cc
X86_SRCS += platform/shared/pal_system_api.cc
X86_SRCS += platform/shared/platform_gnss.cc
X86_SRCS += platform/shared/platform_wifi.cc
X86_SRCS += platform/shared/platform_wwan.cc
X86_SRCS += platform/shared/system_time.cc

GOOGLE_X86_LINUX_SRCS += platform/linux/init.cc

# GoogleTest Compiler Flags ####################################################

GOOGLETEST_CFLAGS += -Iplatform/slpi/include

# GoogleTest Source Files ######################################################

GOOGLETEST_SRCS += platform/linux/assert.cc
GOOGLETEST_SRCS += platform/slpi/smgr/platform_sensor_util.cc
GOOGLETEST_SRCS += platform/slpi/smgr/tests/platform_sensor_util_test.cc
