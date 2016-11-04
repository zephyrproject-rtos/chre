load("//:chre.bzl", "chre_cc_library")
load("//:chre.bzl", "gtest_cc_test")

package(
    default_visibility = [
        "//visibility:public",
    ],
)

chre_cc_library(
    name = "util",
    hdrs = [
        "include/chre/util/array_queue.h",
        "include/chre/util/blocking_queue.h",
        "include/chre/util/entry_points.h",
        "include/chre/util/id_from_string.h",
        "include/chre/util/id_from_string_impl.h",
        "include/chre/util/non_copyable.h",
        "include/chre/util/time.h",
        "include/chre/util/time_impl.h",
        "include/chre/util/dynamic_vector.h",
        "include/chre/util/dynamic_vector_impl.h"
    ],
)

gtest_cc_test(
    name = "blocking_queue_test",
    srcs = [
        "tests/blocking_queue_test.cc",
    ],
)

gtest_cc_test(
    name = "dynamic_vector_test",
    srcs = [
        "tests/dynamic_vector_test.cc",
    ],
    deps = [
        "//util:util",
    ],
)
