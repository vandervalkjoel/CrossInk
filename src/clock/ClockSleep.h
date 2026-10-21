#pragma once

#include <cstddef>
#include <cstdint>

namespace deskclock {

enum class WakeCause {
  Timer,    // slept until the timer
  Input,    // a button or the touch line woke it
  PinBusy,  // a wake pin was already at its wake level, so it did not sleep
};

// Light-sleeps for up to ms milliseconds. RAM, activity state and held power
// pins survive; wakes early on Left, Right, Power or the touch controller's
// interrupt line. Every wake pin is restored before returning. note receives
// one token per wake pin, "<name><gpio>:<level before>/<level after>", for the
// debug log.
// onlyPin >= 0 arms just that one wake pin (by index in the wake pin list)
// plus the timer; -2 arms the timer alone. For diagnosing spurious wakes.
WakeCause lightSleepFor(uint32_t ms, char* note, size_t noteSize, int onlyPin = -1);

// Number of wake pins on this board (for the diagnostic sweep).
int wakePinCount();

}  // namespace deskclock
