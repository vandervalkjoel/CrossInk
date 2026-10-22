#include <gtest/gtest.h>

#include <ctime>

#include "clock/ClockModel.h"

using namespace deskclock;

TEST(ClockWords, NearestFive) {
  EXPECT_EQ(wordsForTime(10, 42).line1, "twenty to");
  EXPECT_EQ(wordsForTime(10, 42).line2, "eleven");
  EXPECT_EQ(wordsForTime(10, 43).line1, "quarter to");
  EXPECT_EQ(wordsForTime(10, 32).line1, "half past");
  EXPECT_EQ(wordsForTime(10, 32).line2, "ten");
  EXPECT_EQ(wordsForTime(10, 57).line1, "five to");
  EXPECT_EQ(wordsForTime(10, 57).line2, "eleven");
  EXPECT_EQ(wordsForTime(10, 58).line1, "eleven");
  EXPECT_EQ(wordsForTime(10, 58).line2, "o'clock");
  EXPECT_EQ(wordsForTime(10, 23).line1, "twenty-five past");
  EXPECT_EQ(wordsForTime(10, 37).line1, "twenty-five to");
  EXPECT_EQ(wordsForTime(13, 15).line1, "quarter past");
  EXPECT_EQ(wordsForTime(13, 15).line2, "one");
  EXPECT_EQ(wordsForTime(12, 30).line2, "twelve");
}

TEST(ClockWords, Midnight) {
  EXPECT_EQ(wordsForTime(23, 58).line1, "twelve");
  EXPECT_EQ(wordsForTime(23, 58).line2, "o'clock");
  EXPECT_EQ(wordsForTime(0, 0).line1, "twelve");
}

TEST(ClockWords, Bucket) {
  EXPECT_EQ(wordsBucket(10, 42), wordsBucket(10, 38));
  EXPECT_NE(wordsBucket(10, 42), wordsBucket(10, 43));
  EXPECT_EQ(wordsBucket(23, 58), wordsBucket(0, 2));
}

TEST(ClockRedraw, MinuteFaces) {
  EXPECT_EQ(secondsUntilNextRedraw({2026, 10, 21, 3, 10, 42, 10}, ClockFace::BigTime), 50);
  EXPECT_EQ(secondsUntilNextRedraw({2026, 10, 21, 3, 10, 42, 59}, ClockFace::DayLine), 1);
  EXPECT_EQ(secondsUntilNextRedraw({2026, 10, 21, 3, 10, 42, 0}, ClockFace::QuietDial), 60);
  EXPECT_EQ(secondsUntilNextRedraw({2026, 10, 21, 3, 23, 59, 30}, ClockFace::BigTime), 30);
}

TEST(ClockRedraw, WordsFace) {
  EXPECT_EQ(secondsUntilNextRedraw({2026, 10, 21, 3, 10, 42, 10}, ClockFace::Words), 50);
  EXPECT_EQ(secondsUntilNextRedraw({2026, 10, 21, 3, 10, 43, 0}, ClockFace::Words), 300);
}

TEST(ClockDial, Angles) {
  EXPECT_FLOAT_EQ(handAngles(10, 42).hourDeg, 321.0f);
  EXPECT_FLOAT_EQ(handAngles(10, 42).minuteDeg, 252.0f);
  EXPECT_FLOAT_EQ(handAngles(0, 0).hourDeg, 0.0f);
  EXPECT_FLOAT_EQ(handAngles(15, 30).hourDeg, 105.0f);
  EXPECT_FLOAT_EQ(handAngles(15, 30).minuteDeg, 180.0f);
}

TEST(ClockDayLine, Fraction) {
  EXPECT_FLOAT_EQ(dayFraction(0, 0), 0.0f);
  EXPECT_NEAR(dayFraction(10, 42), 642.0f / 1440.0f, 1e-6);
  EXPECT_NEAR(dayFraction(23, 59), 1439.0f / 1440.0f, 1e-6);
}

TEST(ClockText, Time) {
  EXPECT_EQ(formatTime(10, 42, false), "10:42");
  EXPECT_EQ(formatTime(9, 5, false), "09:05");
  EXPECT_EQ(formatTime(0, 7, false), "00:07");
  EXPECT_EQ(formatTime(9, 5, true), "9:05");
  EXPECT_EQ(formatTime(0, 7, true), "12:07");
  EXPECT_EQ(formatTime(13, 0, true), "1:00");
}

TEST(ClockText, Date) {
  const ClockTime t{2026, 10, 21, 3, 10, 42, 0};
  EXPECT_EQ(formatDateCaps(t), "WEDNESDAY 21 OCTOBER");
  EXPECT_EQ(formatDateLong(t), "Wednesday, 21 October");
  EXPECT_EQ(weekdayAbbrev(t), "WED");
  EXPECT_EQ(monthAbbrev(t), "OCT");
  EXPECT_EQ(formatDateCaps({2026, 11, 1, 0, 0, 0, 0}), "SUNDAY 1 NOVEMBER");
}

TEST(ClockText, FromTm) {
  std::tm tm{};
  tm.tm_year = 126;
  tm.tm_mon = 9;
  tm.tm_mday = 21;
  tm.tm_wday = 3;
  tm.tm_hour = 10;
  tm.tm_min = 42;
  tm.tm_sec = 5;
  const ClockTime t = fromTm(tm);
  EXPECT_EQ(t.year, 2026);
  EXPECT_EQ(t.month, 10);
  EXPECT_EQ(t.day, 21);
  EXPECT_EQ(t.weekday, 3);
  EXPECT_EQ(t.hour, 10);
  EXPECT_EQ(t.minute, 42);
  EXPECT_EQ(t.second, 5);
}

TEST(ClockPolicy, Refresh) {
  EXPECT_EQ(chooseRefresh(true, 0, 15), RefreshKind::Half);
  EXPECT_EQ(chooseRefresh(false, 14, 15), RefreshKind::Fast);
  EXPECT_EQ(chooseRefresh(false, 15, 15), RefreshKind::Half);
  EXPECT_EQ(fullRefreshMinutes(0), 10);
  EXPECT_EQ(fullRefreshMinutes(1), 15);
  EXPECT_EQ(fullRefreshMinutes(2), 30);
  EXPECT_EQ(fullRefreshMinutes(9), 15);
}

TEST(ClockPolicy, Battery) {
  EXPECT_EQ(batteryState(21), BatteryState::Normal);
  EXPECT_EQ(batteryState(20), BatteryState::Low);
  EXPECT_EQ(batteryState(6), BatteryState::Low);
  EXPECT_EQ(batteryState(5), BatteryState::Critical);
  EXPECT_EQ(batteryState(0), BatteryState::Critical);
}

namespace {
void expectLocal(const ClockTime& t, int year, int month, int day, int weekday, int hour, int minute, int second) {
  EXPECT_EQ(t.year, year);
  EXPECT_EQ(t.month, month);
  EXPECT_EQ(t.day, day);
  EXPECT_EQ(t.weekday, weekday);
  EXPECT_EQ(t.hour, hour);
  EXPECT_EQ(t.minute, minute);
  EXPECT_EQ(t.second, second);
}
}  // namespace

// CrossInk's clockUtcOffsetQ: quarter hours biased by 48 (20 = UTC-7, 48 = UTC, 68 = UTC+5).
TEST(ClockLocal, SameDay) { expectLocal(localFromUtc({2026, 10, 21, 17, 42, 5}, 20), 2026, 10, 21, 3, 10, 42, 5); }

TEST(ClockLocal, PreviousDay) { expectLocal(localFromUtc({2026, 10, 22, 3, 0, 0}, 20), 2026, 10, 21, 3, 20, 0, 0); }

TEST(ClockLocal, PreviousMonth) { expectLocal(localFromUtc({2026, 11, 1, 6, 30, 0}, 20), 2026, 10, 31, 6, 23, 30, 0); }

TEST(ClockLocal, PreviousYear) { expectLocal(localFromUtc({2027, 1, 1, 2, 0, 0}, 20), 2026, 12, 31, 4, 19, 0, 0); }

TEST(ClockLocal, Utc) { expectLocal(localFromUtc({2026, 10, 21, 17, 42, 5}, 48), 2026, 10, 21, 3, 17, 42, 5); }

TEST(ClockRedraw, WordsFaceStopsAtMidnight) {
  // 23:58 and 00:00 to 00:02 share a words bucket; the date still changes.
  EXPECT_EQ(secondsUntilNextRedraw({2026, 10, 21, 3, 23, 58, 0}, ClockFace::Words), 120);
}

TEST(ClockRedraw, KeyChangesWithTheDate) {
  const ClockTime before{2026, 10, 21, 3, 23, 58, 0};
  const ClockTime after{2026, 10, 22, 4, 0, 0, 0};
  EXPECT_NE(redrawKey(before, ClockFace::Words), redrawKey(after, ClockFace::Words));
  EXPECT_NE(redrawKey(before, ClockFace::BigTime), redrawKey(after, ClockFace::BigTime));
  EXPECT_EQ(redrawKey({2026, 10, 21, 3, 10, 42, 0}, ClockFace::Words),
            redrawKey({2026, 10, 21, 3, 10, 38, 0}, ClockFace::Words));
}

TEST(ClockLocal, LeapDay) {
  expectLocal(localFromUtc({2028, 3, 1, 3, 0, 0}, 20), 2028, 2, 29, 2, 20, 0, 0);
}

namespace {
// One pixel per character stands in for a font.
int charWidth(const std::string& s) { return static_cast<int>(s.size()); }
}  // namespace

TEST(ClockWrap, FitsOnOneLine) {
  const auto lines = wrapWords("quarter past", 20, charWidth);
  ASSERT_EQ(lines.size(), 1u);
  EXPECT_EQ(lines[0], "quarter past");
}

TEST(ClockWrap, BreaksAtSpaces) {
  const auto lines = wrapWords("twenty-five past", 12, charWidth);
  ASSERT_EQ(lines.size(), 2u);
  EXPECT_EQ(lines[0], "twenty-five");
  EXPECT_EQ(lines[1], "past");
}

TEST(ClockWrap, BreaksAfterHyphenWhenAWordIsTooWide) {
  const auto lines = wrapWords("twenty-five to", 8, charWidth);
  ASSERT_EQ(lines.size(), 2u);
  EXPECT_EQ(lines[0], "twenty-");
  EXPECT_EQ(lines[1], "five to");
}

TEST(ClockWrap, AWordWiderThanTheLineStaysWhole) {
  const auto lines = wrapWords("eleven", 3, charWidth);
  ASSERT_EQ(lines.size(), 1u);
  EXPECT_EQ(lines[0], "eleven");
}

TEST(ClockLocal, NextYear) { expectLocal(localFromUtc({2026, 12, 31, 21, 0, 0}, 68), 2027, 1, 1, 5, 2, 0, 0); }
