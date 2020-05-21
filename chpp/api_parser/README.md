# CHRE API Parser + Code Generator for CHPP

Since one of the main use cases of CHPP is to support remoting a CHRE PAL
implementation to a peripheral component, we leverage the CHRE PAL API
definitions for the platform-specific implementation of those services, such
as the WWAN service. However, the structures used in the CHRE PAL APIs are the
same as those used in the CHRE API for nanoapps, which are not inherently
serializable, as they include things like pointers to other structures. So we
must create slightly updated versions of these to send over the wire with CHPP.

The `chre_api_to_chpp.py` script in this directory parses CHRE APIs according
to instructions given in `chre_api_annotations.json` and generates structures
and conversion code used for serialization and deserialization.

TODO: add schema definition/documentation for `chre_api_annotations.json`

TODO: add serialization format description (fixed header + offset from
beginning of payload + size)

## Requirements

Written to Python 3.8.2, but most versions of Python 3 should work.

Requires pyclibrary - see requirements.txt.
