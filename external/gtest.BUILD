cc_library(
    name = "main",
    srcs = glob(
        include = [
            "googlemock/src/*.cc",
            "googletest/src/*.cc",
        ],
        exclude = [
            "googlemock/src/gmock-all.cc",
            "googlemock/src/gmock_main.cc",
            "googletest/src/gtest-all.cc",
        ]
    ),
    hdrs = glob([
        "googlemock/include/**/*.h",
        "googletest/include/**/*.h",
        "googletest/src/*.h",
    ]),
    copts = [
        "-Iexternal/gtest/googlemock",
        "-Iexternal/gtest/googletest",
    ],
    defines = [
        "GTEST",
    ],
    includes = [
        "googlemock/include",
        "googletest/include",
    ],
    linkopts = ["-pthread"],
    visibility = ["//visibility:public"],
)
