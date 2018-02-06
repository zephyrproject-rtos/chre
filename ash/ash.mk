#
# ASH Makefile
#

# Common Compiler Flags ########################################################

# Include paths.
COMMON_CFLAGS += -Iash/include

# SLPI/SMGR-specific Source Files ##############################################

SLPI_SMGR_SRCS += ash/platform/slpi/smgr/ash.cc

# Simulator-specific Source Files ##############################################

SIM_SRCS += ash/platform/linux/ash.cc
