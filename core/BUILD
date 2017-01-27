load("//:chre.bzl", "chre_cc_library")
load("//:chre.bzl", "gtest_cc_test")

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
        "include/chre/core/event_loop_manager.h",
        "include/chre/core/timer_pool.h",
    ],
    srcs = [
        "event_loop.cc",
        "event_loop_manager.cc",
        "timer_pool.cc",
    ],
    deps = [
        "//core:event",
        "//core:nanoapp",
        "//core:sensor_request_manager",
        "//platform:fatal_error",
        "//platform:mutex",
        "//platform:system_time",
        "//platform:system_timer",
        "//util",
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
        "//platform:platform_sensor",
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
        "//platform:fatal_error",
        "//platform:platform_nanoapp",
        "//util:util",
    ],
)

chre_cc_library(
    name = "request_multiplexer",
    hdrs = [
        "include/chre/core/request_multiplexer.h",
        "include/chre/core/request_multiplexer_impl.h",
    ],
    deps = [
        "//platform:assert",
        "//util:util",
    ],
)

chre_cc_library(
    name = "sensor",
    hdrs = [
        "include/chre/core/sensor.h",
    ],
    srcs = [
        "sensor.cc",
    ],
    deps = [
        "//platform:platform_sensor",
        "//util:util",
    ],
)

chre_cc_library(
    name = "sensor_request",
    hdrs = [
        "include/chre/core/sensor_request.h",
        "include/chre/core/sensor_request_impl.h",
    ],
    srcs = [
        "sensor_request.cc",
    ],
    deps = [
        "//core:nanoapp",
        "//chre_api:chre_api",
        "//platform:assert",
        "//platform:fatal_error",
        "//util:util",
    ],
)

chre_cc_library(
    name = "sensor_request_manager",
    hdrs = [
        "include/chre/core/sensor_request_manager.h",
    ],
    srcs = [
        "sensor_request_manager.cc",
    ],
    deps = [
        "//core:request_multiplexer",
        "//core:sensor",
        "//core:sensor_request",
        "//util:util",
    ],
)

gtest_cc_test(
    name = "request_multiplexer_test",
    srcs = [
        "tests/request_multiplexer_test.cc",
    ],
    deps = [
        "//core:request_multiplexer",
    ],
)

gtest_cc_test(
    name = "sensor_request_test",
    srcs = [
        "tests/sensor_request_test.cc",
    ],
    deps = [
        "//core:sensor_request",
    ],
)
