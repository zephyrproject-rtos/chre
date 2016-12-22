load("//:chre.bzl", "chre_cc_library")

package(
    default_visibility = [
        "//visibility:public",
    ],
)

chre_cc_library(
    name = "event",
    hdrs = [
        "include/chre/core/event.h",
        "include/chre/core/event_ref_queue.h",
    ],
    deps = [
        "//chre_api:chre_api",
        "//platform:assert",
        "//platform:log",
        "//util:non_copyable",
    ],
)

chre_cc_library(
    name = "event_loop",
    hdrs = [
        "include/chre/core/event_loop.h",
        "include/chre/core/timer_pool.h",
    ],
    srcs = [
        "event_loop.cc",
        "timer_pool.cc",
    ],
    deps = [
        "//core:event",
        "//core:nanoapp",
        "//platform:fatal_error",
        "//platform:system_time",
        "//platform:system_timer",
    ],
)

chre_cc_library(
    name = "init",
    hdrs = [
        "include/chre/core/init.h",
    ],
    srcs = [
        "init.cc",
    ],
    deps = [
        "//platform:platform",
    ],
)

chre_cc_library(
    name = "nanoapp",
    hdrs = [
        "include/chre/core/nanoapp.h",
    ],
    srcs = [
        "nanoapp.cc",
    ],
    deps = [
        "//core:event",
        "//platform:platform_nanoapp",
        "//util:util",
    ],
)
