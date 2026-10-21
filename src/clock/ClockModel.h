#pragma once

// Pure desk-clock logic: no Arduino or hardware dependencies, so every rule
// here is covered by the host tests in test/clock_model.

#include <cstdint>
#include <ctime>
#include <functional>
#include <string>
#include <vector>

namespace deskclock {

// month is 1-12, weekday is 0 = Sunday.
struct ClockTime {
  int year;
  int month;
  int day;
  int weekday;
  int hour;
  int minute;
  int second;
};

ClockTime fromTm(const std::tm& t);

enum class ClockFace : uint8_t { BigTime = 0, QuietDial = 1, Words = 2, DayLine = 3, Count = 4 };
enum class ClockOrientation : uint8_t { Landscape = 0, Portrait = 1, Count = 2 };

// line2 is drawn italic.
struct WordsTime {
  std::string line1;
  std::string line2;
};

// The time in words, to the nearest five minutes.
WordsTime wordsForTime(int hour, int minute);

// 0..287; changes exactly when wordsForTime's result changes.
int wordsBucket(int hour, int minute);

// Greedy line breaking for the words face: breaks at spaces, and after a
// hyphen when a word alone is wider than maxWidth. A word that still does not
// fit stays whole on its own line.
std::vector<std::string> wrapWords(const std::string& text, int maxWidth,
                                   const std::function<int(const std::string&)>& measure);

// Whole seconds until the face next needs drawing (1..300).
int secondsUntilNextRedraw(const ClockTime& t, ClockFace face);

// Degrees clockwise from 12.
struct HandAngles {
  float hourDeg;
  float minuteDeg;
};

HandAngles handAngles(int hour, int minute);

// How far through the day it is, 0 at midnight.
float dayFraction(int hour, int minute);

std::string formatTime(int hour, int minute, bool use12h);
std::string formatDateCaps(const ClockTime& t);  // "WEDNESDAY 21 OCTOBER"
std::string formatDateLong(const ClockTime& t);  // "Wednesday, 21 October"
std::string weekdayAbbrev(const ClockTime& t);   // "WED"
std::string monthAbbrev(const ClockTime& t);     // "OCT"

enum class RefreshKind { Fast, Half };

RefreshKind chooseRefresh(bool firstDraw, int minutesSinceHalf, int intervalMinutes);

// Setting index to minutes: 0 = 10, 1 = 15, 2 = 30, anything else = 15.
int fullRefreshMinutes(uint8_t settingIndex);

enum class BatteryState { Normal, Low, Critical };

BatteryState batteryState(int percent);

// A raw RTC reading (the RTC keeps UTC). month is 1-12.
struct UtcDateTime {
  int year;
  int month;
  int day;
  int hour;
  int minute;
  int second;
};

// Local time from a UTC reading and CrossInk's clockUtcOffsetQ setting
// (quarter hours biased by 48), with the date rolled and the weekday computed.
ClockTime localFromUtc(const UtcDateTime& utc, int utcOffsetQBiased);

}  // namespace deskclock
