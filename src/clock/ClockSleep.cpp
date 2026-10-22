#include "ClockSleep.h"

#ifndef SIMULATOR

#include <Arduino.h>
#include <BoardConfig.h>
#include <driver/gpio.h>
#include <esp_sleep.h>

#include <cstdio>

namespace deskclock {

namespace {
constexpr int MAX_WAKE_PINS = 6;

struct WakePin {
  char name;
  gpio_num_t pin;
  int wakeLevel;  // the level that means "pressed" / "touched"
};

int collectWakePins(WakePin (&pins)[MAX_WAKE_PINS]) {
  const auto& input = BoardConfig::ACTIVE.input;
  int count = 0;
  auto add = [&](const char name, const int8_t pin, const int wakeLevel) {
    if (pin < 0 || count >= MAX_WAKE_PINS) return;
    pins[count++] = WakePin{name, static_cast<gpio_num_t>(pin), wakeLevel};
  };
  // Buttons are active-low with internal pull-ups. The X4 Pro's two side keys
  // are declared as up (GPIO0) and down (GPIO7), not left/right.
  add('L', input.left, LOW);
  add('R', input.right, LOW);
  add('U', input.up, LOW);
  add('D', input.down, LOW);
  add('P', input.power, input.powerActiveHigh ? HIGH : LOW);
  // No touch wake: on the X4 Pro the GT911 pulses its interrupt line every
  // few milliseconds with nobody touching it (a wake-pin sweep on hardware
  // woke after 6 to 38 ms, three runs of three), so arming it means the chip
  // never sleeps. Leaving the clock is by side key or Power instead.
  return count;
}

void describe(const WakePin (&pins)[MAX_WAKE_PINS], const int count, const int (&before)[MAX_WAKE_PINS],
              char* note, const size_t noteSize) {
  if (note == nullptr || noteSize == 0) return;
  size_t used = 0;
  note[0] = '\0';
  for (int i = 0; i < count && used < noteSize; ++i) {
    const int written = snprintf(note + used, noteSize - used, "%s%c%d:%d/%d", i == 0 ? "" : " ", pins[i].name,
                                 static_cast<int>(pins[i].pin), before[i], digitalRead(pins[i].pin));
    if (written < 0) break;
    used += static_cast<size_t>(written);
  }
}
}  // namespace

int wakePinCount() {
  WakePin pins[MAX_WAKE_PINS];
  return collectWakePins(pins);
}

WakeCause lightSleepFor(const uint32_t ms, char* note, const size_t noteSize, const int onlyPin, char* wokePin) {
  if (wokePin != nullptr) *wokePin = 0;
  WakePin pins[MAX_WAKE_PINS];
  int count = collectWakePins(pins);
  if (onlyPin == -2) {
    count = 0;
  } else if (onlyPin >= 0 && onlyPin < count) {
    pins[0] = pins[onlyPin];
    count = 1;
  }
  int before[MAX_WAKE_PINS] = {};
  bool busy = false;
  for (int i = 0; i < count; ++i) {
    before[i] = digitalRead(pins[i].pin);
    if (before[i] == pins[i].wakeLevel) busy = true;
  }
  if (busy) {
    describe(pins, count, before, note, noteSize);
    return WakeCause::PinBusy;
  }

  esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(ms) * 1000ULL);
  for (int i = 0; i < count; ++i) {
    // Keep each wake pin's awake configuration (its pull-up) through the
    // sleep. Added while chasing the instant wakes, which turned out to be the
    // touch line; kept because light sleep is verified on hardware with it.
    gpio_sleep_sel_dis(pins[i].pin);
    gpio_wakeup_enable(pins[i].pin, pins[i].wakeLevel == HIGH ? GPIO_INTR_HIGH_LEVEL : GPIO_INTR_LOW_LEVEL);
  }
  if (count > 0) esp_sleep_enable_gpio_wakeup();

  const esp_err_t slept = esp_light_sleep_start();
  const esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  if (cause == ESP_SLEEP_WAKEUP_GPIO && wokePin != nullptr) {
    for (int i = 0; i < count; ++i) {
      if (digitalRead(pins[i].pin) == pins[i].wakeLevel) {
        *wokePin = pins[i].name;
        break;
      }
    }
  }

  for (int i = 0; i < count; ++i) {
    gpio_wakeup_disable(pins[i].pin);
    // gpio_wakeup_disable() keeps the level type; a later pinMode() would turn
    // it into an interrupt that storms while a button is held.
    gpio_set_intr_type(pins[i].pin, GPIO_INTR_DISABLE);
  }
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

  describe(pins, count, before, note, noteSize);
  if (slept != ESP_OK) return WakeCause::PinBusy;  // refused: back off, do not spin
  return cause == ESP_SLEEP_WAKEUP_GPIO ? WakeCause::Input : WakeCause::Timer;
}

}  // namespace deskclock

#else  // SIMULATOR: no light sleep; behave as if the timer fired.

#include <Arduino.h>

namespace deskclock {
int wakePinCount() { return 0; }

WakeCause lightSleepFor(const uint32_t ms, char* note, const size_t noteSize, int, char* wokePin) {
  if (wokePin != nullptr) *wokePin = 0;
  delay(ms);
  if (note != nullptr && noteSize > 0) note[0] = '\0';
  return WakeCause::Timer;
}
}  // namespace deskclock

#endif
