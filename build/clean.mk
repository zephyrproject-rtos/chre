#
# Cleanup the project by removing the output directory.
#

include build/defs.mk

.PHONY: clean

clean:
	rm -rf $(OUT)
