#pragma once

#include <cstddef>
#include <cstdint>

namespace deskclock {

enum class WakeCause {
  Timer,    // slept until the timer
  Input,    // a button woke it
  PinBusy,  // a wake pin was already at its wake level, or the sleep was refused; it did not sleep
};

// Light-sleeps for up to ms milliseconds. RAM, activity state and held power
// pins survive; wakes early on the side keys or Power (not touch: see
// ClockSleep.cpp). Every wake pin is restored before returning. note receives
// one token per wake pin, "<name><gpio>:<level before>/<level after>", for the
// debug log. wokePin, when given, receives the name of the first pin found at
// its wake level after an Input wake ('U', 'D', 'P', ...), or 0.
// onlyPin >= 0 arms just that one wake pin (by index in the wake pin list)
// plus the timer; -2 arms the timer alone. For diagnosing spurious wakes.
WakeCause lightSleepFor(uint32_t ms, char* note, size_t noteSize, int onlyPin = -1, char* wokePin = nullptr);

// Number of wake pins on this board (for the diagnostic sweep).
int wakePinCount();

}  // namespace deskclock
