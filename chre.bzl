#
# Helper BUILD rules for CHRE targets.
#

# Linux-specific defines.
LINUX_DEFINES = [
    "CHRE_MESSAGE_TO_HOST_MAX_SIZE=2048",
    "CHRE_MINIMUM_LOG_LEVEL=CHRE_LOG_LEVEL_DEBUG",
]

# Select the set of platform-specific defines to add to the build.
PLATFORM_DEFINES = select({
    "//:platform_linux" : LINUX_DEFINES,

    # Default to Linux. This assumes that building elsewhere will blow up.
    # Building without a platform define has no guarantees but is a nice short
    # hand when building on Linux.
    "//conditions:default" : LINUX_DEFINES,
})

ASSERTION_ENABLED_DEFINE = select({
    "//:compilation_mode_opt"       : ["CHRE_ASSERTIONS_DISABLED"],
    "//:compilation_mode_fastbuild" : ["CHRE_ASSERTIONS_ENABLED"],
    "//:compilation_mode_dbg"       : ["CHRE_ASSERTIONS_ENABLED"],
    "//conditions:default"          : ["CHRE_ASSERTIONS_ENABLED"],
})

#
# Creates a cc_binary that has include paths, defines and deps setup correctly.
#
def chre_cc_binary(includes = [], defines = [], **kwargs):
    native.cc_binary(
        defines = defines + PLATFORM_DEFINES + ASSERTION_ENABLED_DEFINE,
        includes = [
            "include",
        ] + includes,
        **kwargs
    )

#
# Creates a cc_library that has include paths, defines and deps setup correctly.
#
def chre_cc_library(includes = [], defines = [], **kwargs):
    native.cc_library(
        defines = defines + PLATFORM_DEFINES + ASSERTION_ENABLED_DEFINE,
        includes = [
            "include",
        ] + includes,
        **kwargs
    )

#
# Creates a gtest cc_test.
#
def gtest_cc_test(copts = [], deps = [], **kwargs):
    native.cc_test(
        copts = copts + [
            "-Iexternal/gtest/googlemock/include",
            "-Iexternal/gtest/googletest/include",
        ],
        deps = deps + [
            "@gtest//:main",
        ],
        **kwargs
    )
