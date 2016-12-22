load("//:chre.bzl", "chre_cc_binary")
load("//:chre.bzl", "chre_cc_library")

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
    "shared/chre_api_version.cc",
    "shared/system_time.cc",
]

chre_cc_library(
    name = "platform",
    hdrs = [
        "include/chre/platform/context.h",
        "include/chre/platform/platform_id.h",
        "include/chre/platform/system_time.h",
    ] + select({
        "//:platform_linux" : LINUX_PLATFORM_HDRS,
        "//conditions:default" : LINUX_PLATFORM_HDRS,
    }),
    srcs = select({
        "//:platform_linux" : LINUX_PLATFORM_SRCS,
        "//conditions:default" : LINUX_PLATFORM_SRCS,
    }),
    includes = select({
        "//:platform_linux" : LINUX_PLATFORM_INCLUDES,
        "//conditions:default" : LINUX_PLATFORM_INCLUDES,
    }),
    linkopts = select({
        "//:platform_linux" : LINUX_PLATFORM_LINKOPTS,
        "//conditions:default" : LINUX_PLATFORM_LINKOPTS,
    }),
    deps = [
        "//core:event_loop",
        "//chre_api:chre_api",
        "//util:non_copyable",
        "//util:util",
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
    hdrs = [
        "include/chre/platform/assert.h",
    ] + select({
        "//:platform_linux" : LINUX_PLATFORM_ASSERT_HDRS,
        "//conditions:default" : LINUX_PLATFORM_ASSERT_HDRS,
    }),
    includes = select({
        "//:platform_linux" : LINUX_PLATFORM_INCLUDES,
        "//conditions:default" : LINUX_PLATFORM_INCLUDES,
    }),
    deps = [
        "//platform:log",
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
        "//:platform_linux" : LINUX_PLATFORM_CONDITION_VARIABLE_HDRS,
        "//conditions:default" : LINUX_PLATFORM_CONDITION_VARIABLE_HDRS,
    }),
    includes = select({
        "//:platform_linux" : LINUX_PLATFORM_INCLUDES,
        "//conditions:default" : LINUX_PLATFORM_INCLUDES,
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
        "//:platform_linux" : LINUX_PLATFORM_FATAL_ERROR_HDRS,
        "//conditions:default" : LINUX_PLATFORM_FATAL_ERROR_HDRS,
    }),
    includes = select({
        "//:platform_linux" : LINUX_PLATFORM_INCLUDES,
        "//conditions:default" : LINUX_PLATFORM_INCLUDES,
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
    "linux/platform_nanoapp.cc",
]

chre_cc_library(
    name = "platform_nanoapp",
    hdrs = [
        "include/chre/platform/platform_nanoapp.h"
    ] + select({
        "//:platform_linux" : LINUX_PLATFORM_NANOAPP_ENTRY_HDRS,
        "//conditions:default" : LINUX_PLATFORM_NANOAPP_ENTRY_HDRS,
    }),
    srcs = select({
        "//:platform_linux" : LINUX_PLATFORM_NANOAPP_ENTRY_SRCS,
        "//conditions:default" : LINUX_PLATFORM_NANOAPP_ENTRY_SRCS,
    }),
    includes = select({
        "//:platform_linux" : LINUX_PLATFORM_INCLUDES,
        "//conditions:default" : LINUX_PLATFORM_INCLUDES,
    }),
    deps = [
        "//util:util",
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
    hdrs = [
        "include/chre/platform/memory.h",
    ],
    srcs = select({
        "//:platform_linux" : LINUX_PLATFORM_MEMORY_SRCS,
        "//conditions:default" : LINUX_PLATFORM_MEMORY_SRCS,
    }),
    includes = select({
        "//:platform_linux" : LINUX_PLATFORM_INCLUDES,
        "//conditions:default" : LINUX_PLATFORM_INCLUDES,
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
        "//:platform_linux" : LINUX_PLATFORM_LOG_HDRS,
        "//conditions:default" : LINUX_PLATFORM_LOG_HDRS,
    }),
    includes = select({
        "//:platform_linux" : LINUX_PLATFORM_INCLUDES,
        "//conditions:default" : LINUX_PLATFORM_INCLUDES,
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
        "//:platform_linux" : LINUX_PLATFORM_MUTEX_HDRS,
        "//conditions:default" : LINUX_PLATFORM_MUTEX_HDRS,
    }),
    includes = select({
        "//:platform_linux" : LINUX_PLATFORM_INCLUDES,
        "//conditions:default" : LINUX_PLATFORM_INCLUDES,
    }),
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
    hdrs = [
        "include/chre/platform/system_time.h",
    ],
    srcs = select({
        "//:platform_linux" : LINUX_PLATFORM_SYSTEM_TIME_SRCS,
        "//conditions:default" : LINUX_PLATFORM_SYSTEM_TIME_SRCS,
    }),
    includes = select({
        "//:platform_linux" : LINUX_PLATFORM_INCLUDES,
        "//conditions:default" : LINUX_PLATFORM_INCLUDES,
    }),
    deps = [
        "//util:util",
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
    hdrs = [
        "include/chre/platform/system_timer.h",
    ] + select({
        "//:platform_linux" : LINUX_PLATFORM_SYSTEM_TIMER_HDRS,
        "//conditions:default" : LINUX_PLATFORM_SYSTEM_TIMER_HDRS,
    }),
    srcs = select({
        "//:platform_linux" : LINUX_PLATFORM_SYSTEM_TIMER_SRCS,
        "//conditions:default" : LINUX_PLATFORM_SYSTEM_TIMER_SRCS,
    }),
    includes = select({
        "//:platform_linux" : LINUX_PLATFORM_INCLUDES,
        "//conditions:default" : LINUX_PLATFORM_INCLUDES,
    }),
    deps = [
        "//util:non_copyable",
        "//util:util",
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
        "//:platform_linux" : LINUX_BINARY_SRCS,
        "//conditions:default" : LINUX_BINARY_SRCS,
    }),
    deps = [
        "//apps/hello_world:hello_world",
        "//apps/timer_world:timer_world",
        "//core:init",
        "//core:nanoapp",
        "//platform:platform",
        "//platform:system_timer",
        "//platform:log",
    ],
)
