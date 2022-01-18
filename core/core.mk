#
# Core Makefile
#

# Common Compiler Flags ########################################################

# Include paths.
COMMON_CFLAGS += -Icore/include

# Common Source Files ##########################################################

COMMON_SRCS += core/debug_dump_manager.cc
COMMON_SRCS += core/event.cc
COMMON_SRCS += core/event_loop.cc
COMMON_SRCS += core/event_loop_manager.cc
COMMON_SRCS += core/event_ref_queue.cc
COMMON_SRCS += core/host_comms_manager.cc
COMMON_SRCS += core/host_notifications.cc
COMMON_SRCS += core/init.cc
COMMON_SRCS += core/log.cc
COMMON_SRCS += core/nanoapp.cc
COMMON_SRCS += core/settings.cc
COMMON_SRCS += core/static_nanoapps.cc
COMMON_SRCS += core/timer_pool.cc

# Optional audio support.
ifeq ($(CHRE_AUDIO_SUPPORT_ENABLED), true)
COMMON_SRCS += core/audio_request_manager.cc
endif

# Optional BLE support.
ifeq ($(CHRE_BLE_SUPPORT_ENABLED), true)
COMMON_SRCS += core/ble_request.cc
COMMON_SRCS += core/ble_request_manager.cc
COMMON_SRCS += core/ble_request_multiplexer.cc
endif

# Optional GNSS support.
ifeq ($(CHRE_GNSS_SUPPORT_ENABLED), true)
COMMON_SRCS += core/gnss_manager.cc
endif

# Optional sensors support.
ifeq ($(CHRE_SENSORS_SUPPORT_ENABLED), true)
COMMON_SRCS += core/sensor.cc
COMMON_SRCS += core/sensor_request.cc
COMMON_SRCS += core/sensor_request_manager.cc
COMMON_SRCS += core/sensor_request_multiplexer.cc
COMMON_SRCS += core/sensor_type.cc
COMMON_SRCS += core/sensor_type_helpers.cc
endif

# Optional Wi-Fi support.
ifeq ($(CHRE_WIFI_SUPPORT_ENABLED), true)
COMMON_SRCS += core/wifi_request_manager.cc
COMMON_SRCS += core/wifi_scan_request.cc
endif

# Optional WWAN support.
ifeq ($(CHRE_WWAN_SUPPORT_ENABLED), true)
COMMON_SRCS += core/wwan_request_manager.cc
endif

# Optional Telemetry support.
ifeq ($(CHRE_TELEMETRY_SUPPORT_ENABLED), true)
COMMON_SRCS += core/telemetry_manager.cc

COMMON_CFLAGS += -DPB_FIELD_32BIT
COMMON_CFLAGS += -DCHRE_TELEMETRY_SUPPORT_ENABLED

NANOPB_EXTENSION = nanopb

NANOPB_SRCS += $(CHRE_PREFIX)/../../hardware/google/pixel/pixelstats/pixelatoms.proto
NANOPB_PROTO_PATH = $(CHRE_PREFIX)/../../hardware/google/pixel/pixelstats/
NANOPB_INCLUDES = $(NANOPB_PROTO_PATH)
NANOPB_FLAGS += --proto_path=$(NANOPB_PROTO_PATH)

include $(CHRE_PREFIX)/build/nanopb.mk
endif

# GoogleTest Source Files ######################################################

GOOGLETEST_SRCS += core/tests/audio_util_test.cc
GOOGLETEST_SRCS += core/tests/ble_request_test.cc
GOOGLETEST_SRCS += core/tests/memory_manager_test.cc
GOOGLETEST_SRCS += core/tests/request_multiplexer_test.cc
GOOGLETEST_SRCS += core/tests/sensor_request_test.cc
GOOGLETEST_SRCS += core/tests/wifi_scan_request_test.cc
