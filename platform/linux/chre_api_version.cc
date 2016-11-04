#include "chre_api/chre/version.h"

#include "chre/util/id_from_string.h"

//! The vendor ID of the Linux variant: "Googl".
constexpr uint64_t kVendorIdGoogle = createIdFromString("Googl");

//! The vendor platform ID of the Linux variant is one. All other
//  Google implementations will be greater than one. Zero is reserved.
constexpr uint64_t kGoogleLinuxPlatformId = 0x001;

//! The patch version of the Linux platform.
constexpr uint32_t kLinuxPatchVersion = 0x0001;

uint32_t chreGetApiVersion(void) {
  return CHRE_API_VERSION_1_1;
}

uint32_t chreGetVersion(void) {
  return chreGetApiVersion() | kLinuxPatchVersion;
}

uint64_t chreGetPlatformId(void) {
  return kVendorIdGoogle | kGoogleLinuxPlatformId;
}
