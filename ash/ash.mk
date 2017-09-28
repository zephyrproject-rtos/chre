#
# ASH Makefile
#

# Common Compiler Flags ########################################################

# Include paths.
COMMON_CFLAGS += -Iash/include

# SLPI-specific Source Files ###################################################

SLPI_SRCS += ash/platform/slpi/smgr/ash.cc

# x86-specific Source Files ####################################################

X86_SRCS += ash/platform/linux/ash.cc
