#include "ClockDebugLog.h"

#include <cstdarg>

// A local, untracked ClockDebugLocal.h containing `#define DESK_CLOCK_DEBUG_LOG`
// switches the trace on for a development build.
#if __has_include("ClockDebugLocal.h")
#include "ClockDebugLocal.h"
#endif

#ifdef DESK_CLOCK_DEBUG_LOG

#include <Arduino.h>
#include <HalStorage.h>
#include <Logging.h>

#include <cstdio>
#include <cstring>

namespace deskclock {

namespace {
constexpr const char* LOG_PATH = "/.crosspoint/desk-clock.log";
constexpr size_t MAX_LOG_BYTES = 64 * 1024;
}  // namespace

void debugLog(const char* format, ...) {
  char line[192];
  const int prefix = snprintf(line, sizeof(line), "%lu ", static_cast<unsigned long>(millis()));
  va_list args;
  va_start(args, format);
  vsnprintf(line + prefix, sizeof(line) - prefix - 1, format, args);
  va_end(args);
  const size_t len = strlen(line);
  line[len] = '\n';

  LOG_INF("CLOCK", "%.*s", static_cast<int>(len), line);

  HalFile file = Storage.open(LOG_PATH, O_WRONLY | O_CREAT | O_APPEND);
  if (!file) return;
  if (file.size() < MAX_LOG_BYTES) file.write(line, len + 1);
  file.close();
}

}  // namespace deskclock

#else

namespace deskclock {
void debugLog(const char*, ...) {}
}  // namespace deskclock

#endif
