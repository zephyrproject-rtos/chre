#
# Test Makefile
#

# TODO (b/175919480) - General improvements to make adding tests easier
# - How best to configure it at build time? For example, is it sufficient
#   to supply an env var when invoking make, or would a different build
#   target be useful (similar to how we have *_debug targets)?
# - How can we add tests for one specific platform without impacting a
#   different platform where it doesn't apply (and may not compile)?
# - Can we also loop in tests for common code, which are currently
#   executed off-target via ./run_tests.sh (those added via GOOGLETEST_SRCS,
#   e.g. core/tests/*, util/tests/*)

ifeq ($(CHRE_ON_DEVICE_TESTS_ENABLED),true)
include $(CHRE_PREFIX)/build/pw_unit_test.mk
COMMON_CFLAGS += $(PW_UT_CFLAGS)
COMMON_CFLAGS += -I$(CHRE_PREFIX)/test/common/include
COMMON_SRCS += $(CHRE_PREFIX)/test/common/run_tests.cc

COMMON_CFLAGS += -DCHRE_ON_DEVICE_TESTS_ENABLED

COMMON_SRCS += $(PW_UT_SRCS)

# Test Sources Follow!

endif
