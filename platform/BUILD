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

LINUX_PLATFORM_HDRS = [
    "linux/include/chre/target_platform/condition.h",
    "linux/include/chre/target_platform/system_timer_base.h",
]

LINUX_PLATFORM_INCLUDES = [
    "linux/include",
]

LINUX_PLATFORM_LINKOPTS = [
    "-lrt",
]

LINUX_PLATFORM_SRCS = [
    "linux/chre_api_core.cc",
    "linux/chre_api_re.cc",
    "linux/chre_api_version.cc",
    "linux/system_time.cc",
    "linux/system_timer.cc",
    "shared/chre_api_re.cc",
    "shared/memory.cc",
    "shared/system_time.cc",
]

chre_cc_library(
    name = "platform",
    hdrs = [
        "include/chre/platform/condition.h",
        "include/chre/platform/context.h",
        "include/chre/platform/memory.h",
        "include/chre/platform/mutex.h",
        "include/chre/platform/system_time.h",
        "include/chre/platform/system_timer.h",
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
        "//core:memory_pool",
        "//chre_api:chre_api",
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
        "//core:init",
        "//core:nanoapp",
        "//platform:platform",
        "//platform:log",
    ],
)
