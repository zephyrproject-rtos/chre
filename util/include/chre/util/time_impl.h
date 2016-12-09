#include "chre/util/time.h"

//! The number of nanoseconds in one second.
constexpr uint64_t kOneSecondInNanoseconds(1000000000);

//! The number of nanoseconds in one millisecond.
constexpr uint64_t kOneMillisecondInNanoseconds(1000000);

namespace chre {

constexpr Seconds::Seconds(uint64_t seconds)
    : mSeconds(seconds) {}

constexpr uint64_t Seconds::toRawNanoseconds() const {
  // Perform the simple unit conversion. Warning: overflow is caught and
  // handled by returning UINT64_MAX. A ternary expression is used because
  // constexpr requires it.
  return mSeconds > (UINT64_MAX / kOneSecondInNanoseconds) ? UINT64_MAX
      : mSeconds * kOneSecondInNanoseconds;
}

constexpr Milliseconds::Milliseconds(uint64_t milliseconds)
    : mMilliseconds(milliseconds) {}

constexpr uint64_t Milliseconds::toRawNanoseconds() const {
  // Perform the simple unit conversion. Warning: overflow is caught and
  // handled by returning UINT64_MAX. A ternary expression is used because
  // constexpr requires it.
  return mMilliseconds > (UINT64_MAX / kOneMillisecondInNanoseconds) ? UINT64_MAX
      : mMilliseconds * kOneMillisecondInNanoseconds;
}

constexpr Nanoseconds::Nanoseconds()
    : mNanoseconds(0) {}

constexpr Nanoseconds::Nanoseconds(uint64_t nanoseconds)
    : mNanoseconds(nanoseconds) {}

constexpr Nanoseconds::Nanoseconds(Milliseconds milliseconds)
    : mNanoseconds(milliseconds.toRawNanoseconds()) {}

constexpr uint64_t Nanoseconds::toRawNanoseconds() const {
  return mNanoseconds;
}

constexpr Nanoseconds operator+(const Seconds& secs,
                                const Nanoseconds& nanos) {
  return Nanoseconds(secs.toRawNanoseconds() + nanos.toRawNanoseconds());
}

constexpr Nanoseconds operator+(const Nanoseconds& nanos_a,
                                const Nanoseconds& nanos_b) {
  return Nanoseconds(nanos_a.toRawNanoseconds() + nanos_b.toRawNanoseconds());
}

constexpr Nanoseconds operator-(const Nanoseconds& nanos_a,
                                const Nanoseconds& nanos_b) {
  return Nanoseconds(nanos_a.toRawNanoseconds() - nanos_b.toRawNanoseconds());
}

constexpr bool operator>=(const Nanoseconds& nanos_a,
                          const Nanoseconds& nanos_b) {
  return nanos_a.toRawNanoseconds() >= nanos_b.toRawNanoseconds();
}

constexpr bool operator<(const Nanoseconds& nanos_a,
                         const Nanoseconds& nanos_b) {
  return nanos_a.toRawNanoseconds() < nanos_b.toRawNanoseconds();
}

constexpr bool operator>(const Nanoseconds& nanos_a,
                         const Nanoseconds& nanos_b) {
  return nanos_a.toRawNanoseconds() > nanos_b.toRawNanoseconds();
}

}  // namespace chre
