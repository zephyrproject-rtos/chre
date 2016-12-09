load("//:chre.bzl", "chre_cc_library")

package(
    default_visibility = [
        "//visibility:public",
    ],
)

chre_cc_library(
    name = "timer_world",
    srcs = [
        "timer_world.cc",
    ],
    hdrs = [
        "include/chre/apps/timer_world/timer_world.h",
    ],
    deps = [
        "//chre_api:chre_api",
    ],
)
