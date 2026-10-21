#pragma once

class GfxRenderer;

namespace deskclock {

// Font ids for the clock's own fonts (src/clock/fonts). Fixed non-zero values
// that do not collide with the generated ids in src/fontIds.h.
constexpr int CLOCK_DIGITS_XL_FONT_ID = 1129072401;  // big time
constexpr int CLOCK_DIGITS_L_FONT_ID = 1129072402;   // day line time
constexpr int CLOCK_DIGITS_M_FONT_ID = 1129072403;   // dial date number
constexpr int CLOCK_CAPS_FONT_ID = 1129072404;       // letter-spaced capitals
constexpr int CLOCK_WORDS_FONT_ID = 1129072405;      // words face (regular + italic)
constexpr int CLOCK_SERIF_SMALL_FONT_ID = 1129072406;

// Inserts the clock fonts into the renderer once per boot; safe to call on
// every entry to the clock.
void ensureFontsLoaded(GfxRenderer& renderer);

}  // namespace deskclock
