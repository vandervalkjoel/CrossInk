#pragma once

// Optional desk-clock trace on the SD card (/.crosspoint/desk-clock.log).
// USB serial drops while the clock light-sleeps, so wake causes are written
// here instead. Compiled in only when DESK_CLOCK_DEBUG_LOG is defined (see
// ClockDebugLog.cpp); otherwise every call is a no-op.
namespace deskclock {

void debugLog(const char* format, ...) __attribute__((format(printf, 1, 2)));

}  // namespace deskclock
