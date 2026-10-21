#include "ClockActivity.h"

#include <GfxRenderer.h>
#include <HalClock.h>
#include <HalPowerManager.h>
#include <I18n.h>
#include <Logging.h>

#include <cstdio>
#include <string>

#include "CrossPointSettings.h"
#include "MappedInputManager.h"
#include "clock/ClockDebugLog.h"
#include "clock/ClockSleep.h"
#include "fontIds.h"

#if __has_include("ClockDebugLocal.h")
#include "ClockDebugLocal.h"  // local, untracked development switches
#endif

using namespace deskclock;

namespace {
constexpr uint32_t NOT_SET_RETRY_MS = 60 * 1000;
constexpr uint32_t PIN_BUSY_RETRY_MS = 50;
constexpr uint32_t WAKE_MARGIN_MS = 500;
constexpr int KEY_TIME_NOT_SET = -2;

// The RTC keeps UTC; CrossInk's clockUtcOffsetQ turns it into wall time.
// A clock whose date has never been synced holds an arbitrary date, so it
// counts as not set (CrossInk gates its own date display the same way).
bool readLocalTime(ClockTime& out) {
  if (!SETTINGS.clockDateHasBeenSynced) return false;
  uint16_t year = 0;
  uint8_t month = 0, day = 0, hour = 0, minute = 0, second = 0;
  if (!halClock.getDateTimeNow(year, month, day, hour, minute, second)) return false;
  out = localFromUtc({year, month, day, hour, minute, second}, SETTINGS.clockUtcOffsetQ);
  return true;
}

int redrawKey(const bool valid, const ClockTime& t, const ClockFace face) {
  if (!valid) return KEY_TIME_NOT_SET;
  if (face == ClockFace::Words) return wordsBucket(t.hour, t.minute);
  return t.hour * 60 + t.minute;
}

const char* causeName(const WakeCause cause) {
  switch (cause) {
    case WakeCause::Timer:
      return "timer";
    case WakeCause::Input:
      return "input";
    case WakeCause::PinBusy:
      return "pin-busy";
  }
  return "?";
}
}  // namespace

void ClockActivity::onEnter() {
  Activity::onEnter();
  face = SETTINGS.clockFace < static_cast<uint8_t>(ClockFace::Count) ? static_cast<ClockFace>(SETTINGS.clockFace)
                                                                       : ClockFace::BigTime;
  orientation = SETTINGS.clockOrientation < static_cast<uint8_t>(ClockOrientation::Count)
                    ? static_cast<ClockOrientation>(SETTINGS.clockOrientation)
                    : ClockOrientation::Landscape;
  fullRefreshInterval = fullRefreshMinutes(SETTINGS.clockFullRefresh);
  use12h = SETTINGS.clockFormat == 1;
  previousOrientation = renderer.getOrientation();
  renderer.setOrientation(orientation == ClockOrientation::Portrait ? GfxRenderer::Orientation::Portrait
                                                                    : GfxRenderer::Orientation::LandscapeClockwise);
  firstDraw = true;
  lastHalfMinutes = 0;
  lastDrawnKey = -1;
  exiting = false;
  sweepDone = false;
  debugLog("open face=%d orientation=%d heap=%u", static_cast<int>(face), static_cast<int>(orientation),
           static_cast<unsigned>(ESP.getFreeHeap()));
}

void ClockActivity::onExit() {
  renderer.setOrientation(previousOrientation);
  Activity::onExit();
}

void ClockActivity::exitClock(const char* why) {
  if (exiting) return;
  exiting = true;
  debugLog("close: %s", why);
  onGoHome();
}

void ClockActivity::loop() {
  if (exiting) return;

  int x = 0;
  int y = 0;
  // The X4 Pro's two side keys arrive as Up/Down (page back/forward) rather
  // than Left/Right, so accept any of them.
  using Button = MappedInputManager::Button;
  for (const Button button : {Button::Left, Button::Right, Button::Up, Button::Down, Button::PageBack,
                              Button::PageForward}) {
    if (mappedInput.wasPressed(button)) return exitClock("button");
  }
  if (mappedInput.wasScreenTapped(x, y)) return exitClock("tap");

  ClockTime t{};
  const bool valid = readLocalTime(t);
  const int key = redrawKey(valid, t, face);
  if (key != lastDrawnKey) {
    timeValid = valid;
    shown = t;
    if (requestUpdateAndWait() == RequestUpdateResult::Rejected) {
      debugLog("render rejected");
      return;  // try again on the next loop
    }
    lastDrawnKey = key;
  }

#ifdef DESK_CLOCK_WAKE_SWEEP
  // Diagnostic: once per open, sleep 3 s with each wake pin armed alone, then
  // with the timer alone, and log which ones end the sleep early.
  if (!sweepDone) {
    sweepDone = true;
    char sweepPins[96];
    for (int i = -2; i < wakePinCount(); ++i) {
      if (i == -1) continue;
      const unsigned long started = millis();
      const WakeCause sweepCause = lightSleepFor(3000, sweepPins, sizeof(sweepPins), i);
      debugLog("sweep arm=%d cause=%s slept=%lu pins=%s", i, causeName(sweepCause), millis() - started, sweepPins);
    }
  }
#endif

  const uint32_t sleepMs =
      valid ? static_cast<uint32_t>(secondsUntilNextRedraw(t, face)) * 1000 + WAKE_MARGIN_MS : NOT_SET_RETRY_MS;
  char pins[96];
  const WakeCause cause = lightSleepFor(sleepMs, pins, sizeof(pins));
  debugLog("wake %s planned=%lu pins=%s", causeName(cause), static_cast<unsigned long>(sleepMs), pins);

  if (cause == WakeCause::PinBusy) {
    // A contact is closing now; let the main loop read it before sleeping.
    delay(PIN_BUSY_RETRY_MS);
    return;
  }
  if (cause == WakeCause::Input) exitClock("wake input");
}

void ClockActivity::render(RenderLock&&) {
  renderer.clearScreen();
  const int midY = renderer.getScreenHeight() / 2;

  if (!timeValid) {
    renderer.drawCenteredText(UI_12_FONT_ID, midY, tr(STR_CLOCK_TIME_NOT_SET));
  } else {
    const std::string time = formatTime(shown.hour, shown.minute, use12h);
    const std::string date = formatDateLong(shown);
    renderer.drawCenteredText(UI_12_FONT_ID, midY - 20, time.c_str(), true, EpdFontFamily::BOLD);
    renderer.drawCenteredText(UI_12_FONT_ID, midY + 20, date.c_str());
  }

  // Debug aid for the battery measurement; removed when the faces land.
  char battery[16];
  snprintf(battery, sizeof(battery), "%u%%", static_cast<unsigned>(powerManager.getBatteryPercentage()));
  renderer.drawText(UI_10_FONT_ID, 10, renderer.getScreenHeight() - 30, battery);

  const int nowMinutes = shown.hour * 60 + shown.minute;
  const int minutesSinceHalf = (nowMinutes - lastHalfMinutes + 24 * 60) % (24 * 60);
  const RefreshKind kind = chooseRefresh(firstDraw || !timeValid, minutesSinceHalf, fullRefreshInterval);
  renderer.displayBuffer(kind == RefreshKind::Half ? HalDisplay::HALF_REFRESH : HalDisplay::FAST_REFRESH);
  if (kind == RefreshKind::Half) lastHalfMinutes = nowMinutes;
  firstDraw = false;
}
