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
        "include/chre/util/array_queue_impl.h",
        "include/chre/util/dynamic_vector.h",
        "include/chre/util/dynamic_vector_impl.h",
        "include/chre/util/entry_points.h",
        "include/chre/util/fixed_size_blocking_queue.h",
        "include/chre/util/fixed_size_blocking_queue_impl.h",
        "include/chre/util/fixed_size_vector.h",
        "include/chre/util/fixed_size_vector_impl.h",
        "include/chre/util/heap.h",
        "include/chre/util/heap_impl.h",
        "include/chre/util/id_from_string.h",
        "include/chre/util/id_from_string_impl.h",
        "include/chre/util/lock_guard.h",
        "include/chre/util/lock_guard_impl.h",
        "include/chre/util/memory_pool.h",
        "include/chre/util/memory_pool_impl.h",
        "include/chre/util/priority_queue.h",
        "include/chre/util/priority_queue_impl.h",
        "include/chre/util/singleton.h",
        "include/chre/util/singleton_impl.h",
        "include/chre/util/synchronized_memory_pool.h",
        "include/chre/util/synchronized_memory_pool_impl.h",
        "include/chre/util/time.h",
        "include/chre/util/time_impl.h",
    ],
    deps = [
        "//platform:assert",
        "//platform:condition_variable",
        "//platform:log",
        "//platform:memory",
        "//platform:mutex",
        "//util:non_copyable",
    ],
)

chre_cc_library(
    name = "non_copyable",
    hdrs = [
        "include/chre/util/non_copyable.h",
    ],
)

gtest_cc_test(
    name = "blocking_queue_test",
    srcs = [
        "tests/blocking_queue_test.cc",
    ],
    deps = [
        "//util",
    ],
)

gtest_cc_test(
    name = "dynamic_vector_test",
    srcs = [
        "tests/dynamic_vector_test.cc",
    ],
    deps = [
        "//util",
    ],
)

gtest_cc_test(
    name = "fixed_size_vector_test",
    srcs = [
        "tests/fixed_size_vector_test.cc",
    ],
    deps = [
        "//util",
    ],
)

gtest_cc_test(
    name = "memory_pool_test",
    srcs = [
        "tests/memory_pool_test.cc",
    ],
    deps = [
        "//util",
    ],
)

gtest_cc_test(
    name = "array_queue_test",
    srcs = [
        "tests/array_queue_test.cc",
    ],
    deps = [
        "//util",
    ],
)

gtest_cc_test(
    name = "lock_guard_test",
    srcs = [
        "tests/lock_guard_test.cc",
    ],
    deps = [
        "//util",
    ],
)

gtest_cc_test(
    name = "priority_queue_test",
    srcs = [
        "tests/priority_queue_test.cc",
    ],
    deps = [
        "//util",
    ],
)

gtest_cc_test(
    name = "heap_test",
    srcs = [
        "tests/heap_test.cc",
    ],
    deps = [
        "//util",
    ],
)

gtest_cc_test(
    name = "singleton_test",
    srcs = [
        "tests/singleton_test.cc",
    ],
    deps = [
        "//util",
    ],
)
