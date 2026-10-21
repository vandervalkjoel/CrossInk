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
WakeCause lightSleepFor(uint32_t ms, char* note, size_t noteSize);

}  // namespace deskclock
