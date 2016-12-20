#
# Platform Makefile
#

# Common Compiler Flags ########################################################

# Include paths.
COMMON_CFLAGS += -Iplatform/include

# Common Source Files ##########################################################

COMMON_SRCS += platform/slpi/init.cc
COMMON_SRCS += platform/slpi/system_time.cc

# Hexagon-specific Compiler Flags ##############################################

# Include paths.
HEXAGON_CFLAGS += -Iplatform/slpi/include

# Symbols required by the runtime for conditional compilation.
HEXAGON_CFLAGS += -DCHRE_MINIMUM_LOG_LEVEL=CHRE_LOG_LEVEL_DEBUG

# Hexagon-specific Source Files ################################################

HEXAGON_SRCS += platform/shared/system_time.cc
