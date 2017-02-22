load("//:chre.bzl", "chre_cc_binary")
load("//:chre.bzl", "chre_cc_library")
load("//:chre.bzl", "gtest_cc_test")

package(
    default_visibility = [
        "//visibility:public",
    ],
)

################################################################################
#
# Platform library
#
################################################################################

LINUX_PLATFORM_INCLUDES = [
    "linux/include",
]

LINUX_PLATFORM_LINKOPTS = [
    "-lrt",
]

LINUX_PLATFORM_HDRS = [
    "linux/include/chre/target_platform/platform_id_impl.h",
]

LINUX_PLATFORM_SRCS = [
    "linux/chre_api_re.cc",
    "linux/system_time.cc",
    "shared/chre_api_core.cc",
    "shared/chre_api_re.cc",
    "shared/chre_api_sensor.cc",
    "shared/chre_api_version.cc",
    "shared/chre_api_wifi.cc",
    "shared/system_time.cc",
]

chre_cc_library(
    name = "platform",
    srcs = select({
        "//:platform_linux": LINUX_PLATFORM_SRCS,
        "//conditions:default": LINUX_PLATFORM_SRCS,
    }),
    hdrs = [
        "include/chre/platform/context.h",
        "include/chre/platform/platform_id.h",
        "include/chre/platform/system_time.h",
    ] + select({
        "//:platform_linux": LINUX_PLATFORM_HDRS,
        "//conditions:default": LINUX_PLATFORM_HDRS,
    }),
    includes = select({
        "//:platform_linux": LINUX_PLATFORM_INCLUDES,
        "//conditions:default": LINUX_PLATFORM_INCLUDES,
    }),
    linkopts = select({
        "//:platform_linux": LINUX_PLATFORM_LINKOPTS,
        "//conditions:default": LINUX_PLATFORM_LINKOPTS,
    }),
    deps = [
        "//chre_api",
        "//core:event_loop",
        "//util",
        "//util:non_copyable",
    ],
)

################################################################################
#
# Platform Assert
#
################################################################################

LINUX_PLATFORM_ASSERT_HDRS = [
    "linux/include/chre/target_platform/assert.h",
]

chre_cc_library(
    name = "assert",
    srcs = [
        "linux/assert.cc",
    ],
    hdrs = [
        "include/chre/platform/assert.h",
    ] + select({
        "//:platform_linux": LINUX_PLATFORM_ASSERT_HDRS,
        "//conditions:default": LINUX_PLATFORM_ASSERT_HDRS,
    }),
    includes = select({
        "//:platform_linux": LINUX_PLATFORM_INCLUDES,
        "//conditions:default": LINUX_PLATFORM_INCLUDES,
    }),
    deps = [
        "//platform:log",
        "@gtest//:main",
    ],
)

################################################################################
#
# Platform Condition Variable
#
################################################################################

LINUX_PLATFORM_CONDITION_VARIABLE_HDRS = [
    "linux/include/chre/target_platform/condition_variable_base.h",
    "linux/include/chre/target_platform/condition_variable_impl.h",
]

chre_cc_library(
    name = "condition_variable",
    hdrs = [
        "include/chre/platform/condition_variable.h",
    ] + select({
        "//:platform_linux": LINUX_PLATFORM_CONDITION_VARIABLE_HDRS,
        "//conditions:default": LINUX_PLATFORM_CONDITION_VARIABLE_HDRS,
    }),
    includes = select({
        "//:platform_linux": LINUX_PLATFORM_INCLUDES,
        "//conditions:default": LINUX_PLATFORM_INCLUDES,
    }),
)

################################################################################
#
# Platform Fatal Error
#
################################################################################

LINUX_PLATFORM_FATAL_ERROR_HDRS = [
    "linux/include/chre/target_platform/fatal_error.h",
]

chre_cc_library(
    name = "fatal_error",
    hdrs = [
        "include/chre/platform/fatal_error.h",
    ] + select({
        "//:platform_linux": LINUX_PLATFORM_FATAL_ERROR_HDRS,
        "//conditions:default": LINUX_PLATFORM_FATAL_ERROR_HDRS,
    }),
    includes = select({
        "//:platform_linux": LINUX_PLATFORM_INCLUDES,
        "//conditions:default": LINUX_PLATFORM_INCLUDES,
    }),
    deps = [
        "//platform:log",
    ],
)

################################################################################
#
# Platform Nanoapp
#
################################################################################

LINUX_PLATFORM_NANOAPP_ENTRY_HDRS = [
    "linux/include/chre/target_platform/platform_nanoapp_base.h",
]

LINUX_PLATFORM_NANOAPP_ENTRY_SRCS = [
    "shared/platform_nanoapp.cc",
]

chre_cc_library(
    name = "platform_nanoapp",
    srcs = select({
        "//:platform_linux": LINUX_PLATFORM_NANOAPP_ENTRY_SRCS,
        "//conditions:default": LINUX_PLATFORM_NANOAPP_ENTRY_SRCS,
    }),
    hdrs = [
        "include/chre/platform/platform_nanoapp.h",
    ] + select({
        "//:platform_linux": LINUX_PLATFORM_NANOAPP_ENTRY_HDRS,
        "//conditions:default": LINUX_PLATFORM_NANOAPP_ENTRY_HDRS,
    }),
    includes = select({
        "//:platform_linux": LINUX_PLATFORM_INCLUDES,
        "//conditions:default": LINUX_PLATFORM_INCLUDES,
    }),
    deps = [
        "//util",
    ],
)

################################################################################
#
# Platform Memory
#
################################################################################

LINUX_PLATFORM_MEMORY_SRCS = [
    "shared/memory.cc",
]

chre_cc_library(
    name = "memory",
    srcs = select({
        "//:platform_linux": LINUX_PLATFORM_MEMORY_SRCS,
        "//conditions:default": LINUX_PLATFORM_MEMORY_SRCS,
    }),
    hdrs = [
        "include/chre/platform/memory.h",
        "include/chre/platform/memory_impl.h",
    ],
    includes = select({
        "//:platform_linux": LINUX_PLATFORM_INCLUDES,
        "//conditions:default": LINUX_PLATFORM_INCLUDES,
    }),
)

################################################################################
#
# Platform Logging
#
################################################################################

LINUX_PLATFORM_LOG_HDRS = [
    "linux/include/chre/target_platform/log.h",
]

chre_cc_library(
    name = "log",
    hdrs = [
        "include/chre/platform/log.h",
    ] + select({
        "//:platform_linux": LINUX_PLATFORM_LOG_HDRS,
        "//conditions:default": LINUX_PLATFORM_LOG_HDRS,
    }),
    includes = select({
        "//:platform_linux": LINUX_PLATFORM_INCLUDES,
        "//conditions:default": LINUX_PLATFORM_INCLUDES,
    }),
)

################################################################################
#
# Platform Mutex
#
################################################################################

LINUX_PLATFORM_MUTEX_HDRS = [
    "linux/include/chre/target_platform/mutex_base.h",
    "linux/include/chre/target_platform/mutex_base_impl.h",
]

chre_cc_library(
    name = "mutex",
    hdrs = [
        "include/chre/platform/mutex.h",
    ] + select({
        "//:platform_linux": LINUX_PLATFORM_MUTEX_HDRS,
        "//conditions:default": LINUX_PLATFORM_MUTEX_HDRS,
    }),
    includes = select({
        "//:platform_linux": LINUX_PLATFORM_INCLUDES,
        "//conditions:default": LINUX_PLATFORM_INCLUDES,
    }),
)

################################################################################
#
# Platform Wifi PAL
#
################################################################################

LINUX_PLATFORM_PAL_WIFI_SRCS = [
    "shared/pal_wifi_stub.cc",
]

chre_cc_library(
    name = "pal_wifi",
    srcs = select({
        "//:platform_linux": LINUX_PLATFORM_PAL_WIFI_SRCS,
        "//conditions:default": LINUX_PLATFORM_PAL_WIFI_SRCS,
    }),
    deps = [
        "//pal",
    ],
)

################################################################################
#
# Platform Sensor
#
################################################################################

LINUX_PLATFORM_PLATFORM_SENSOR_SRCS = [
    "linux/platform_sensor.cc",
]

LINUX_PLATFORM_PLATFORM_SENSOR_HDRS = [
    "linux/include/chre/target_platform/platform_sensor_base.h",
]

chre_cc_library(
    name = "platform_sensor",
    srcs = select({
        "//:platform_linux": LINUX_PLATFORM_PLATFORM_SENSOR_SRCS,
        "//conditions:default": LINUX_PLATFORM_PLATFORM_SENSOR_SRCS,
    }),
    hdrs = [
        "include/chre/platform/platform_sensor.h",
    ] + select({
        "//:platform_linux": LINUX_PLATFORM_PLATFORM_SENSOR_HDRS,
        "//conditions:default": LINUX_PLATFORM_PLATFORM_SENSOR_HDRS,
    }),
    includes = select({
        "//:platform_linux": LINUX_PLATFORM_INCLUDES,
        "//conditions:default": LINUX_PLATFORM_INCLUDES,
    }),
    deps = [
        "//core:sensor_request",
        "//util",
    ],
)

################################################################################
#
# Platform Sensor Util (SLPI-only)
#
################################################################################

chre_cc_library(
    name = "slpi_platform_sensor_util",
    srcs = [
        "slpi/platform_sensor_util.cc",
    ],
    hdrs = [
        "slpi/include/chre/platform/slpi/platform_sensor_util.h",
    ],
    includes = [
        "slpi/include",
    ],
    deps = [
        "//util",
    ],
)

gtest_cc_test(
    name = "slpi_platform_sensor_util_test",
    srcs = [
        "slpi/tests/platform_sensor_util_test.cc",
    ],
    deps = [
        "//platform:slpi_platform_sensor_util",
    ],
)

################################################################################
#
# Platform System Time
#
################################################################################

LINUX_PLATFORM_SYSTEM_TIME_SRCS = [
    "linux/system_time.cc",
]

chre_cc_library(
    name = "system_time",
    srcs = select({
        "//:platform_linux": LINUX_PLATFORM_SYSTEM_TIME_SRCS,
        "//conditions:default": LINUX_PLATFORM_SYSTEM_TIME_SRCS,
    }),
    hdrs = [
        "include/chre/platform/system_time.h",
    ],
    includes = select({
        "//:platform_linux": LINUX_PLATFORM_INCLUDES,
        "//conditions:default": LINUX_PLATFORM_INCLUDES,
    }),
    deps = [
        "//util",
    ],
)

################################################################################
#
# Platform System Timer
#
################################################################################

LINUX_PLATFORM_SYSTEM_TIMER_HDRS = [
    "linux/include/chre/target_platform/system_timer_base.h",
]

LINUX_PLATFORM_SYSTEM_TIMER_SRCS = [
    "linux/system_timer.cc",
]

chre_cc_library(
    name = "system_timer",
    srcs = select({
        "//:platform_linux": LINUX_PLATFORM_SYSTEM_TIMER_SRCS,
        "//conditions:default": LINUX_PLATFORM_SYSTEM_TIMER_SRCS,
    }),
    hdrs = [
        "include/chre/platform/system_timer.h",
    ] + select({
        "//:platform_linux": LINUX_PLATFORM_SYSTEM_TIMER_HDRS,
        "//conditions:default": LINUX_PLATFORM_SYSTEM_TIMER_HDRS,
    }),
    includes = select({
        "//:platform_linux": LINUX_PLATFORM_INCLUDES,
        "//conditions:default": LINUX_PLATFORM_INCLUDES,
    }),
    deps = [
        "//util",
        "//util:non_copyable",
    ],
)

################################################################################
#
# Platform Binary
#
################################################################################

LINUX_BINARY_SRCS = [
    "linux/init.cc",
]

chre_cc_binary(
    name = "chre",
    srcs = select({
        "//:platform_linux": LINUX_BINARY_SRCS,
        "//conditions:default": LINUX_BINARY_SRCS,
    }),
    deps = [
        "//apps/hello_world",
        "//apps/sensor_world",
        "//apps/timer_world",
        "//core:init",
        "//core:nanoapp",
        "//platform",
        "//platform:log",
        "//platform:system_timer",
    ],
)
