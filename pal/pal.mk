#
# PAL Makefile
#

# Common Compiler Flags ########################################################

# Include paths.
COMMON_CFLAGS += -Ipal/include

# GoogleTest Source Files ######################################################

GOOGLETEST_SRCS += pal/tests/version_test.cc
GOOGLETEST_SRCS += pal/tests/wwan_test.cc
GOOGLETEST_PAL_IMPL_SRCS += pal/tests/wifi_pal_impl_test.cc
