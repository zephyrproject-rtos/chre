#
# Core Makefile
#

# Common Compiler Flags ########################################################

# Include paths.
COMMON_CFLAGS += -Icore/include

# Common Source Files ##########################################################

COMMON_SRCS += core/event_loop.cc
COMMON_SRCS += core/init.cc
COMMON_SRCS += core/nanoapp.cc
COMMON_SRCS += core/sensors.cc
COMMON_SRCS += core/timer_pool.cc
