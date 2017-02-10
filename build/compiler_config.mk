#
# Common global compiler configuration
#

# Common Compiler Flags ########################################################

# CHRE requires C++11 support.
# TODO: Add support for CXX_SRCS and C_SRCS and break this out.
COMMON_CFLAGS += -std=c++11

# Configure warnings.
COMMON_CFLAGS += -Wall
COMMON_CFLAGS += -Werror

# Disable exceptions and RTTI.
COMMON_CFLAGS += -fno-exceptions
COMMON_CFLAGS += -fno-rtti

# Enable the linker to garbage collect unused code and variables.
COMMON_CFLAGS += -fdata-sections
COMMON_CFLAGS += -ffunction-sections
