#
# All build artifacts are output in a directory called out/ at the root of the
# project.
#

include build/defs.mk

#
# Ensure that the output directory exists.
#
$(OUT):
	mkdir -p $(OUT)
