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
HEXAGON_CFLAGS += -DCHRE_MESSAGE_TO_HOST_MAX_SIZE=2048
HEXAGON_CFLAGS += -DCHRE_ASSERTIONS_ENABLED

# Hexagon-specific Source Files ################################################

HEXAGON_SRCS += platform/shared/chre_api_core.cc
HEXAGON_SRCS += platform/shared/chre_api_re.cc
HEXAGON_SRCS += platform/shared/chre_api_version.cc
HEXAGON_SRCS += platform/shared/system_time.cc
HEXAGON_SRCS += platform/slpi/system_timer.cc
