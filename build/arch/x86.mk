#
# Build targets for an x86 processor
#

# x86 Environment Checks #######################################################

ifeq ($(X86_TOOLS_PREFIX),)
$(error "You should supply an X86_TOOLS_PREFIX environment variable \
         containing a path to the x86 toolchain. This is typically provided by \
         the Android source tree. Example: export X86_TOOLS_PREFIX=$$HOME/\
         android/master/prebuilts/clang/host/linux-x86/clang-3688880/bin/")
endif

# x86 Tools ####################################################################

TARGET_AR  = $(X86_TOOLS_PREFIX)llvm-ar
TARGET_CC  = $(X86_TOOLS_PREFIX)clang++
TARGET_LD  = $(X86_TOOLS_PREFIX)clang++

# x86 Compiler Flags ###########################################################

# Add x86 compiler flags.
TARGET_CFLAGS += $(X86_CFLAGS)

# Enable position independence.
TARGET_CFLAGS += -fpic

# x86 Shared Object Linker Flags ###############################################

TARGET_SO_LDFLAGS += -shared

# Optimization Level ###########################################################

TARGET_CFLAGS += -O$(OPT_LEVEL)

# Variant Specific Sources #####################################################

TARGET_VARIANT_SRCS += $(X86_SRCS)
