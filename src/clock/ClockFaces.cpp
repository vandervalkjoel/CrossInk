#include "ClockFaces.h"

#include <GfxRenderer.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "clock/ClockFonts.h"
#include "fontIds.h"

namespace deskclock {

namespace {

constexpr float kPi = 3.14159265f;

struct Area {
  int x;
  int y;
  int width;
  int height;
  int centerX() const { return x + width / 2; }
  int centerY() const { return y + height / 2; }
  int right() const { return x + width; }
  int bottom() const { return y + height; }
};

// The part of the screen not hidden by the bezel, in the current orientation.
Area viewable(const GfxRenderer& r) {
  int top = 0, right = 0, bottom = 0, left = 0;
  r.getOrientedViewableTRBL(&top, &right, &bottom, &left);
  return Area{left, top, r.getScreenWidth() - left - right, r.getScreenHeight() - top - bottom};
}

// Visible height of text (glyph ink, not the font's line box).
int inkHeight(const GfxRenderer& r, const int font, const char* text) {
  const auto b = r.getTextVerticalBounds(font, text);
  return b.bottom - b.top;
}

// Draws text so its visible ink starts at inkTop.
void drawAtInkTop(const GfxRenderer& r, const int font, const int x, const int inkTop, const char* text,
                  const EpdFontFamily::Style style = EpdFontFamily::REGULAR) {
  const auto b = r.getTextVerticalBounds(font, text);
  r.drawText(font, x, inkTop - b.top, text, true, style);
}

int width(const GfxRenderer& r, const int font, const char* text,
          const EpdFontFamily::Style style = EpdFontFamily::REGULAR) {
  return r.getTextWidth(font, text, style);
}

// Capitals with extra space between letters, as in the mockups.
int spacedWidth(const GfxRenderer& r, const int font, const std::string& text, const int spacing) {
  int w = 0;
  for (size_t i = 0; i < text.size(); ++i) {
    const char glyph[2] = {text[i], '\0'};
    w += text[i] == ' ' ? r.getSpaceWidth(font) : width(r, font, glyph);
    if (i + 1 < text.size()) w += spacing;
  }
  return w;
}

void drawSpaced(const GfxRenderer& r, const int font, int x, const int inkTop, const std::string& text,
                const int spacing) {
  const auto b = r.getTextVerticalBounds(font, "H");
  for (const char c : text) {
    const char glyph[2] = {c, '\0'};
    if (c == ' ') {
      x += r.getSpaceWidth(font) + spacing;
      continue;
    }
    r.drawText(font, x, inkTop - b.top, glyph);
    x += width(r, font, glyph) + spacing;
  }
}

int capsHeight(const GfxRenderer& r) { return inkHeight(r, CLOCK_CAPS_FONT_ID, "H"); }

void fillDisc(const GfxRenderer& r, const int cx, const int cy, const int radius) {
  constexpr int kPoints = 24;
  int xs[kPoints];
  int ys[kPoints];
  for (int i = 0; i < kPoints; ++i) {
    const float a = 2.0f * kPi * static_cast<float>(i) / kPoints;
    xs[i] = cx + static_cast<int>(std::lround(radius * std::cos(a)));
    ys[i] = cy + static_cast<int>(std::lround(radius * std::sin(a)));
  }
  r.fillPolygon(xs, ys, kPoints, true);
}

void pointOnDial(const int cx, const int cy, const float degrees, const float length, int& x, int& y) {
  const float a = degrees * kPi / 180.0f;
  x = cx + static_cast<int>(std::lround(length * std::sin(a)));
  y = cy - static_cast<int>(std::lround(length * std::cos(a)));
}

// Small battery outline with the percentage, bottom right, only when low.
void drawLowBattery(const GfxRenderer& r, const Area& area, const int percent) {
  char text[8];
  snprintf(text, sizeof(text), "%d%%", percent);
  const int textWidth = width(r, UI_10_FONT_ID, text);
  const int bodyW = 26, bodyH = 13, margin = 12;
  const int x = area.right() - margin - bodyW - 4 - textWidth - 8;
  const int y = area.bottom() - margin - bodyH;
  r.drawRect(x, y, bodyW, bodyH, 2, true);
  r.fillRect(x + bodyW, y + 4, 3, bodyH - 8, true);
  const int fill = std::max(1, (bodyW - 6) * std::clamp(percent, 0, 100) / 100);
  r.fillRect(x + 3, y + 3, fill, bodyH - 6, true);
  drawAtInkTop(r, UI_10_FONT_ID, x + bodyW + 4 + 8, y, text);
}

std::string hours(const ClockTime& t, const bool use12h) {
  const std::string s = formatTime(t.hour, t.minute, use12h);
  return s.substr(0, s.find(':'));
}

std::string minutes(const ClockTime& t) {
  char buf[4];
  snprintf(buf, sizeof(buf), "%02d", t.minute);
  return buf;
}

// 1. Big time: the time as large as it fits, the date in spaced capitals.
void drawBigTime(const GfxRenderer& r, const Area& a, const ClockOrientation o, const FaceContext& c) {
  const int capsH = capsHeight(r);
  constexpr int spacing = 4;
  if (o == ClockOrientation::Landscape) {
    const std::string time = formatTime(c.time.hour, c.time.minute, c.use12h);
    const std::string date = formatDateCaps(c.time);
    const int timeH = inkHeight(r, CLOCK_DIGITS_XL_FONT_ID, time.c_str());
    const int gap = 44;
    const int top = a.centerY() - (timeH + gap + capsH) / 2;
    drawAtInkTop(r, CLOCK_DIGITS_XL_FONT_ID, a.centerX() - width(r, CLOCK_DIGITS_XL_FONT_ID, time.c_str()) / 2, top,
                 time.c_str());
    drawSpaced(r, CLOCK_CAPS_FONT_ID, a.centerX() - spacedWidth(r, CLOCK_CAPS_FONT_ID, date, spacing) / 2,
               top + timeH + gap, date, spacing);
    return;
  }
  // Portrait: hours over minutes, then the date on two lines.
  const std::string h = hours(c.time, c.use12h);
  const std::string m = minutes(c.time);
  const std::string weekday = formatDateCaps(c.time).substr(0, formatDateCaps(c.time).find(' '));
  char dayMonth[24];
  snprintf(dayMonth, sizeof(dayMonth), "%d %s", c.time.day,
           formatDateCaps(c.time).substr(formatDateCaps(c.time).rfind(' ') + 1).c_str());
  const int digitH = inkHeight(r, CLOCK_DIGITS_XL_FONT_ID, "0");
  const int gap = 36, lineGap = 16;
  const int total = digitH * 2 + gap + 56 + capsH * 2 + lineGap;
  int y = a.centerY() - total / 2;
  drawAtInkTop(r, CLOCK_DIGITS_XL_FONT_ID, a.centerX() - width(r, CLOCK_DIGITS_XL_FONT_ID, h.c_str()) / 2, y,
               h.c_str());
  y += digitH + gap;
  drawAtInkTop(r, CLOCK_DIGITS_XL_FONT_ID, a.centerX() - width(r, CLOCK_DIGITS_XL_FONT_ID, m.c_str()) / 2, y,
               m.c_str());
  y += digitH + 56;
  drawSpaced(r, CLOCK_CAPS_FONT_ID, a.centerX() - spacedWidth(r, CLOCK_CAPS_FONT_ID, weekday, spacing) / 2, y,
             weekday, spacing);
  y += capsH + lineGap;
  drawSpaced(r, CLOCK_CAPS_FONT_ID, a.centerX() - spacedWidth(r, CLOCK_CAPS_FONT_ID, dayMonth, spacing) / 2, y,
             dayMonth, spacing);
}

void drawDial(const GfxRenderer& r, const int cx, const int cy, const int radius, const ClockTime& t) {
  for (int i = 0; i < 12; ++i) {
    const bool quarter = i % 3 == 0;
    const float deg = static_cast<float>(i) * 30.0f;
    int x1, y1, x2, y2;
    pointOnDial(cx, cy, deg, static_cast<float>(radius) - (quarter ? 30.0f : 18.0f), x1, y1);
    pointOnDial(cx, cy, deg, static_cast<float>(radius), x2, y2);
    r.drawLine(x1, y1, x2, y2, quarter ? 6 : 3, true);
  }
  const HandAngles hands = handAngles(t.hour, t.minute);
  int hx, hy, mx, my;
  pointOnDial(cx, cy, hands.hourDeg, radius * 0.52f, hx, hy);
  pointOnDial(cx, cy, hands.minuteDeg, radius * 0.80f, mx, my);
  r.drawLine(cx, cy, hx, hy, 10, true);
  r.drawLine(cx, cy, mx, my, 5, true);
  fillDisc(r, cx, cy, 9);
}

// 2. Quiet dial: ticks and two hands, no numerals; the date set off beside it.
void drawQuietDial(const GfxRenderer& r, const Area& a, const ClockOrientation o, const FaceContext& c) {
  const std::string weekday = weekdayAbbrev(c.time);
  const std::string month = monthAbbrev(c.time);
  char day[4];
  snprintf(day, sizeof(day), "%d", c.time.day);
  const int capsH = capsHeight(r);
  const int dayH = inkHeight(r, CLOCK_DIGITS_M_FONT_ID, day);
  constexpr int spacing = 5;
  const int lineGap = 22;
  const int stackH = capsH + lineGap + dayH + lineGap + capsH;

  if (o == ClockOrientation::Landscape) {
    const int radius = std::min(a.height / 2 - 40, a.width / 3 - 40);
    const int cx = a.x + a.width * 34 / 100;
    const int cy = a.centerY();
    drawDial(r, cx, cy, radius, c.time);
    const int ruleX = a.x + a.width * 68 / 100;
    r.drawLine(ruleX, cy - radius * 3 / 4, ruleX, cy + radius * 3 / 4, 2, true);
    const int colX = ruleX + (a.right() - ruleX) / 2;
    int y = cy - stackH / 2;
    drawSpaced(r, CLOCK_CAPS_FONT_ID, colX - spacedWidth(r, CLOCK_CAPS_FONT_ID, weekday, spacing) / 2, y, weekday,
               spacing);
    y += capsH + lineGap;
    drawAtInkTop(r, CLOCK_DIGITS_M_FONT_ID, colX - width(r, CLOCK_DIGITS_M_FONT_ID, day) / 2, y, day);
    y += dayH + lineGap;
    drawSpaced(r, CLOCK_CAPS_FONT_ID, colX - spacedWidth(r, CLOCK_CAPS_FONT_ID, month, spacing) / 2, y, month,
               spacing);
    return;
  }
  // Portrait: dial on top, the date beneath on one row.
  const int radius = std::min(a.width / 2 - 40, a.height * 3 / 10);
  const int cy = a.y + a.height * 36 / 100;
  drawDial(r, a.centerX(), cy, radius, c.time);
  const int rowY = cy + radius + 70;
  const int wW = spacedWidth(r, CLOCK_CAPS_FONT_ID, weekday, spacing);
  const int dW = width(r, CLOCK_DIGITS_M_FONT_ID, day);
  const int mW = spacedWidth(r, CLOCK_CAPS_FONT_ID, month, spacing);
  constexpr int itemGap = 28;
  int x = a.centerX() - (wW + itemGap + dW + itemGap + mW) / 2;
  const int capsTop = rowY + (dayH - capsH) / 2;
  drawSpaced(r, CLOCK_CAPS_FONT_ID, x, capsTop, weekday, spacing);
  x += wW + itemGap;
  drawAtInkTop(r, CLOCK_DIGITS_M_FONT_ID, x, rowY, day);
  x += dW + itemGap;
  drawSpaced(r, CLOCK_CAPS_FONT_ID, x, capsTop, month, spacing);
}

// 3. Words: the time in words, the hour in italic, a short rule and the date.
void drawWords(const GfxRenderer& r, const Area& a, const ClockOrientation o, const FaceContext& c) {
  const WordsTime words = wordsForTime(c.time.hour, c.time.minute);
  const int margin = o == ClockOrientation::Landscape ? 70 : 40;
  const int maxWidth = a.width - margin * 2;
  auto measureRegular = [&r](const std::string& s) { return width(r, CLOCK_WORDS_FONT_ID, s.c_str()); };
  auto measureItalic = [&r](const std::string& s) {
    return width(r, CLOCK_WORDS_FONT_ID, s.c_str(), EpdFontFamily::ITALIC);
  };
  const std::vector<std::string> regular = wrapWords(words.line1, maxWidth, measureRegular);
  const std::vector<std::string> italic = wrapWords(words.line2, maxWidth, measureItalic);
  const std::string date = formatDateLong(c.time);

  const int lineH = r.getLineHeight(CLOCK_WORDS_FONT_ID);
  const int lines = static_cast<int>(regular.size() + italic.size());
  const int ruleGap = 34, dateGap = 24;
  const int dateH = r.getLineHeight(CLOCK_SERIF_SMALL_FONT_ID);
  const int total = lines * lineH + ruleGap + 2 + dateGap + dateH;
  int y = a.centerY() - total / 2;
  const int x = a.x + margin;
  for (const std::string& line : regular) {
    r.drawText(CLOCK_WORDS_FONT_ID, x, y, line.c_str());
    y += lineH;
  }
  for (const std::string& line : italic) {
    r.drawText(CLOCK_WORDS_FONT_ID, x, y, line.c_str(), true, EpdFontFamily::ITALIC);
    y += lineH;
  }
  y += ruleGap;
  r.fillRect(x, y, 90, 2, true);
  y += 2 + dateGap;
  r.drawText(CLOCK_SERIF_SMALL_FONT_ID, x, y, date.c_str());
}

// 4. Day line: the time and date, and a line showing how far through the day it is.
void drawDayLine(const GfxRenderer& r, const Area& a, const ClockOrientation o, const FaceContext& c) {
  const bool landscape = o == ClockOrientation::Landscape;
  const int margin = landscape ? 70 : 40;
  const std::string time = formatTime(c.time.hour, c.time.minute, c.use12h);
  const std::string date = formatDateCaps(c.time);
  constexpr int spacing = 4;
  const int timeFont = landscape ? CLOCK_DIGITS_L_FONT_ID : CLOCK_DIGITS_M_FONT_ID;
  const int timeH = inkHeight(r, timeFont, time.c_str());
  const int capsH = capsHeight(r);
  const int x = a.x + margin;

  const int lineY = a.bottom() - (landscape ? 90 : 120);
  const int textBlock = timeH + 36 + capsH;
  const int top = a.y + (lineY - a.y - textBlock) / 2;
  drawAtInkTop(r, timeFont, x, top, time.c_str());
  if (landscape) {
    drawSpaced(r, CLOCK_CAPS_FONT_ID, x + 4, top + timeH + 36, date, spacing);
  } else {
    // Portrait is too narrow for the full date on one line.
    const std::string weekday = date.substr(0, date.find(' '));
    const std::string rest = date.substr(date.find(' ') + 1);
    drawSpaced(r, CLOCK_CAPS_FONT_ID, x + 2, top + timeH + 36, weekday, spacing);
    drawSpaced(r, CLOCK_CAPS_FONT_ID, x + 2, top + timeH + 36 + capsH + 14, rest, spacing);
  }

  const int x0 = a.x + margin;
  const int x1 = a.right() - margin;
  const int now = x0 + static_cast<int>(std::lround((x1 - x0) * dayFraction(c.time.hour, c.time.minute)));
  r.fillRect(x0, lineY - 1, x1 - x0, 2, true);
  r.fillRect(x0, lineY - 3, std::max(0, now - x0), 6, true);
  const char* labels[] = {"00", "06", "12", "18"};
  for (int h = 0; h <= 24; h += 6) {
    const int tx = x0 + (x1 - x0) * h / 24;
    r.fillRect(tx - 1, lineY - 10, 2, 20, true);
    if (h < 24) {
      const char* label = labels[h / 6];
      drawAtInkTop(r, UI_10_FONT_ID, tx - width(r, UI_10_FONT_ID, label) / 2, lineY + 22, label);
    }
  }
  fillDisc(r, now, lineY, 10);
}

}  // namespace

void drawFace(GfxRenderer& renderer, const ClockFace face, const ClockOrientation orientation,
              const FaceContext& ctx) {
  ensureFontsLoaded(renderer);
  const Area area = viewable(renderer);
  switch (face) {
    case ClockFace::QuietDial:
      drawQuietDial(renderer, area, orientation, ctx);
      break;
    case ClockFace::Words:
      drawWords(renderer, area, orientation, ctx);
      break;
    case ClockFace::DayLine:
      drawDayLine(renderer, area, orientation, ctx);
      break;
    case ClockFace::BigTime:
    case ClockFace::Count:
    default:
      drawBigTime(renderer, area, orientation, ctx);
      break;
  }
  if (ctx.lowBattery) drawLowBattery(renderer, area, ctx.batteryPercent);
}

}  // namespace deskclock
