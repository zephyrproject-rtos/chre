load("//:chre.bzl", "chre_cc_library")

package(
    default_visibility = [
        "//visibility:public",
    ],
)

chre_cc_library(
    name = "hello_world",
    srcs = [
        "hello_world.cc",
    ],
    hdrs = [
        "include/chre/apps/hello_world/hello_world.h",
    ],
    deps = [
        "//chre_api:chre_api",
    ],
)
