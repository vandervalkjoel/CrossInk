#include "ClockFonts.h"

#include <EpdFont.h>
#include <EpdFontFamily.h>
#include <GfxRenderer.h>

#include "clock/fonts/clock_caps.h"
#include "clock/fonts/clock_digits_l.h"
#include "clock/fonts/clock_digits_m.h"
#include "clock/fonts/clock_digits_xl.h"
#include "clock/fonts/clock_serif_small.h"
#include "clock/fonts/clock_serif_words.h"
#include "clock/fonts/clock_serif_words_italic.h"

namespace deskclock {

namespace {
EpdFont digitsXl(&clock_digits_xl);
EpdFont digitsL(&clock_digits_l);
EpdFont digitsM(&clock_digits_m);
EpdFont caps(&clock_caps);
EpdFont words(&clock_serif_words);
EpdFont wordsItalic(&clock_serif_words_italic);
EpdFont serifSmall(&clock_serif_small);

EpdFontFamily digitsXlFamily(&digitsXl);
EpdFontFamily digitsLFamily(&digitsL);
EpdFontFamily digitsMFamily(&digitsM);
EpdFontFamily capsFamily(&caps);
EpdFontFamily wordsFamily(&words, nullptr, &wordsItalic);
EpdFontFamily serifSmallFamily(&serifSmall);
}  // namespace

void ensureFontsLoaded(GfxRenderer& renderer) {
  static bool loaded = false;
  if (loaded) return;
  renderer.insertFont(CLOCK_DIGITS_XL_FONT_ID, digitsXlFamily);
  renderer.insertFont(CLOCK_DIGITS_L_FONT_ID, digitsLFamily);
  renderer.insertFont(CLOCK_DIGITS_M_FONT_ID, digitsMFamily);
  renderer.insertFont(CLOCK_CAPS_FONT_ID, capsFamily);
  renderer.insertFont(CLOCK_WORDS_FONT_ID, wordsFamily);
  renderer.insertFont(CLOCK_SERIF_SMALL_FONT_ID, serifSmallFamily);
  loaded = true;
}

}  // namespace deskclock
