load("//:chre.bzl", "chre_cc_library")

package(
    default_visibility = [
        "//visibility:public",
    ],
)

chre_cc_library(
    name = "sensor_world",
    srcs = [
        "sensor_world.cc",
    ],
    hdrs = [
        "include/chre/apps/sensor_world/sensor_world.h",
    ],
    deps = [
        "//chre_api:chre_api",
        "//util:util",
    ],
)
