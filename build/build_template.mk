#
# Build Template
#
# Invoke this template with a set of variables in order to make build targets
# for a build variant that targets a specific CPU architecture.
#

include $(CHRE_PREFIX)/build/defs.mk

################################################################################
#
# Build Template
#
# Invoke this to instantiate a set of build targets. Two build targets are
# produced by this template that can be either used directly or depended on to
# perform post processing (ie: during a nanoapp build).
#
# TARGET_NAME_ar - An archive of the code compiled by this template.
# TARGET_NAME_so - A shared object of the code compiled by this template.
# TARGET_NAME    - A convenience target that depends on the above archive and
#                  shared object targets.
#
# Argument List:
#     $1 - TARGET_NAME         - The name of the target being built.
#     $2 - TARGET_CFLAGS       - The compiler flags to use for this target.
#     $3 - TARGET_CC           - The C/C++ compiler for the target variant.
#     $4 - TARGET_LDFLAGS      - The linker flags to use for this target.
#     $5 - TARGET_LD           - The linker for the target variant.
#     $6 - TARGET_ARFLAGS      - The archival flags to use for this target.
#     $7 - TARGET_AR           - The archival tool for the targer variant.
#     $8 - TARGET_VARIANT_SRCS - Source files specific to this variant.
#
################################################################################

ifndef BUILD_TEMPLATE
define BUILD_TEMPLATE

# Target Objects ###############################################################

# Source files.
$$(1)_SRCS = $(COMMON_SRCS) $(8)

# Object files.
$$(1)_OBJS_DIR = $(1)_objs
$$(1)_OBJS = $$(patsubst %.cc, $(OUT)/$$($$(1)_OBJS_DIR)/%.o, $$($$(1)_SRCS))

# Add object file directories.
$$$(1)_DIRS = $$(sort $$(dir $$($$(1)_OBJS)))

# Outputs ######################################################################

# Shared Object
$$(1)_SO = $(OUT)/$$$(1)/$(OUTPUT_NAME).so

# Static Archive
$$(1)_AR = $(OUT)/$$$(1)/$(OUTPUT_NAME).a

# Top-level Build Rule #########################################################

# Define the phony target.
.PHONY: $(1)_ar
$(1)_ar: $$($$(1)_AR)

.PHONY: $(1)_so
$(1)_so: $$($$(1)_SO)

.PHONY: $(1)
$(1): $(1)_ar $(1)_so

# If building the runtime, simply add the archive and shared object to the all
# target. When building CHRE, it is expected that this runtime just be linked
# into another build system (or the entire runtime is built using another build
# system).
ifeq ($(IS_NANOAPP_BUILD),)
all: $(1)
endif

# Compile ######################################################################

# Add common and target-specific compiler flags.
$$$(1)_CFLAGS = $(COMMON_CFLAGS) \
    $(2)

$$($$(1)_OBJS): $(OUT)/$$($$(1)_OBJS_DIR)/%.o: %.cc
	$(3) $$($$$(1)_CFLAGS) -c $$< -o $$@

# Archive ######################################################################

$$($$(1)_AR): $$(OUT)/$$$(1) $$($$$(1)_DIRS) $$($$(1)_OBJS)
	$(7) $(6) $$@ $$(filter %.o, $$^)

# Link #########################################################################

$$($$(1)_SO): $$(OUT)/$$$(1) $$($$$(1)_DIRS) $$($$(1)_OBJS)
	$(5) $(4) -o $$@ $$(filter %.o, $$^)

# Output Directories ###########################################################

$$($$$(1)_DIRS):
	mkdir -p $$@

$$(OUT)/$$$(1):
	mkdir -p $$@

endef
endif

# Template Invocation ##########################################################

$(eval $(call BUILD_TEMPLATE, $(TARGET_NAME), \
                              $(TARGET_CFLAGS), \
                              $(TARGET_CC), \
                              $(TARGET_LDFLAGS), \
                              $(TARGET_LD), \
                              $(TARGET_ARFLAGS), \
                              $(TARGET_AR), \
                              $(TARGET_VARIANT_SRCS)))
