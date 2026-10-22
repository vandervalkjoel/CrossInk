#pragma once

#include "activities/Activity.h"
#include "clock/ClockModel.h"

// Desk clock: shows the time and date, redraws once a minute and light-sleeps
// in between. A side key returns to the home screen; Power keeps its normal
// meaning. A tap only registers in the moments the chip is awake, because the
// touch line cannot be a wake source (see ClockSleep.cpp). Never uses Wi-Fi:
// time comes from the RTC, which CrossInk keeps in sync elsewhere.
class ClockActivity final : public Activity {
 public:
  ClockActivity(GfxRenderer& renderer, MappedInputManager& mappedInput) : Activity("Clock", renderer, mappedInput) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;
  bool preventAutoSleep() override { return true; }

 private:
  void exitClock(const char* why);

  GfxRenderer::Orientation previousOrientation = GfxRenderer::Orientation::Portrait;

  // Read from settings on every entry.
  deskclock::ClockFace face = deskclock::ClockFace::BigTime;
  deskclock::ClockOrientation orientation = deskclock::ClockOrientation::Landscape;
  int fullRefreshInterval = 15;
  bool use12h = false;

  // Written by loop() before requestUpdateAndWait(), read by render().
  bool timeValid = false;
  deskclock::ClockTime shown{};
  bool firstDraw = true;
  int lastHalfMinutes = 0;  // minute of day of the last HALF refresh

  int lastDrawnKey = -1;
  bool exiting = false;
  bool sweepDone = false;  // DESK_CLOCK_WAKE_SWEEP diagnostic only
  bool rtcRetried = false;
};
