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
        "//util:util",
    ],
)

chre_cc_library(
    name = "event_loop",
    hdrs = [
        "include/chre/core/event_loop.h",
    ],
    srcs = [
        "event_loop.cc",
    ],
    deps = [
        "//core:event",
        "//core:memory_pool",
        "//core:nanoapp",
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
    name = "memory_pool",
    hdrs = [
        "include/chre/core/memory_pool.h",
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
    ],
)

chre_cc_library(
    name = "timer",
    hdrs = [
        "include/chre/core/timer.h",
    ],
    deps = [
        "//chre_api:chre_api",
        "//util:util",
    ],
)
