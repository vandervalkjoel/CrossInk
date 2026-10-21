#include "ClockModel.h"

#include <cstdio>

namespace deskclock {

namespace {
constexpr int MINUTES_PER_DAY = 24 * 60;
constexpr int WORDS_BUCKETS = MINUTES_PER_DAY / 5;

// Days since 1970-01-01 for a proleptic Gregorian date (Howard Hinnant's days_from_civil).
int64_t daysFromCivil(int y, const int m, const int d) {
  y -= m <= 2;
  const int64_t era = (y >= 0 ? y : y - 399) / 400;
  const int64_t yoe = y - era * 400;
  const int64_t doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const int64_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + doe - 719468;
}

// Inverse of daysFromCivil.
void civilFromDays(int64_t z, int& y, int& m, int& d) {
  z += 719468;
  const int64_t era = (z >= 0 ? z : z - 146096) / 146097;
  const int64_t doe = z - era * 146097;
  const int64_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  const int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  const int64_t mp = (5 * doy + 2) / 153;
  d = static_cast<int>(doy - (153 * mp + 2) / 5 + 1);
  m = static_cast<int>(mp < 10 ? mp + 3 : mp - 9);
  y = static_cast<int>(yoe + era * 400 + (m <= 2));
}

const char* const HOUR_WORDS[12] = {"twelve", "one", "two",   "three", "four",   "five",
                                    "six",    "seven", "eight", "nine",  "ten", "eleven"};

// Index i is minutes i * 5; index 0 (o'clock) is handled separately.
const char* const PHRASES[12] = {"",          "five past",        "ten past",  "quarter past",
                                 "twenty past", "twenty-five past", "half past", "twenty-five to",
                                 "twenty to", "quarter to",       "ten to",    "five to"};

const char* const WEEKDAYS[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const char* const MONTHS[12] = {"January", "February", "March",     "April",   "May",      "June",
                                "July",    "August",   "September", "October", "November", "December"};

std::string upper(std::string s) {
  for (char& c : s) {
    if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
  }
  return s;
}

const char* weekdayName(const ClockTime& t) { return WEEKDAYS[((t.weekday % 7) + 7) % 7]; }
const char* monthName(const ClockTime& t) { return MONTHS[((t.month - 1) % 12 + 12) % 12]; }
}  // namespace

ClockTime fromTm(const std::tm& t) {
  return ClockTime{t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_wday, t.tm_hour, t.tm_min, t.tm_sec};
}

WordsTime wordsForTime(const int hour, const int minute) {
  int rounded = (minute + 2) / 5 * 5;
  int h = hour;
  if (rounded == 60) {
    rounded = 0;
    h += 1;
  }
  const int step = rounded / 5;
  if (step == 0) return {HOUR_WORDS[h % 12], "o'clock"};
  const int named = step > 6 ? h + 1 : h;  // "to" phrases name the next hour
  return {PHRASES[step], HOUR_WORDS[named % 12]};
}

std::vector<std::string> wrapWords(const std::string& text, const int maxWidth,
                                   const std::function<int(const std::string&)>& measure) {
  // Split into words, then split any too-wide word after its hyphens.
  std::vector<std::string> tokens;
  size_t start = 0;
  while (start < text.size()) {
    size_t end = text.find(' ', start);
    if (end == std::string::npos) end = text.size();
    const std::string word = text.substr(start, end - start);
    if (!word.empty()) {
      if (measure(word) > maxWidth && word.find('-') != std::string::npos) {
        size_t partStart = 0;
        size_t hyphen;
        while ((hyphen = word.find('-', partStart)) != std::string::npos) {
          tokens.push_back(word.substr(partStart, hyphen + 1 - partStart));
          partStart = hyphen + 1;
        }
        if (partStart < word.size()) tokens.push_back(word.substr(partStart));
      } else {
        tokens.push_back(word);
      }
    }
    start = end + 1;
  }

  std::vector<std::string> lines;
  std::string line;
  bool lineEndsWithHyphen = false;
  for (const std::string& token : tokens) {
    if (line.empty()) {
      line = token;
    } else {
      // A piece split off after a hyphen joins without a space.
      const std::string candidate = line + (lineEndsWithHyphen ? "" : " ") + token;
      if (measure(candidate) <= maxWidth) {
        line = candidate;
      } else {
        lines.push_back(line);
        line = token;
      }
    }
    lineEndsWithHyphen = !line.empty() && line.back() == '-';
  }
  if (!line.empty()) lines.push_back(line);
  return lines;
}

int wordsBucket(const int hour, const int minute) { return ((hour * 60 + minute + 2) / 5) % WORDS_BUCKETS; }

int secondsUntilNextRedraw(const ClockTime& t, const ClockFace face) {
  const int toNextMinute = 60 - t.second;
  if (face != ClockFace::Words) return toNextMinute;
  const int now = t.hour * 60 + t.minute;
  const int bucket = wordsBucket(t.hour, t.minute);
  int minutesAhead = 1;
  while (minutesAhead < 5) {
    const int m = (now + minutesAhead) % MINUTES_PER_DAY;
    if (wordsBucket(m / 60, m % 60) != bucket) break;
    ++minutesAhead;
  }
  return toNextMinute + (minutesAhead - 1) * 60;
}

HandAngles handAngles(const int hour, const int minute) {
  return {(static_cast<float>(hour % 12) + static_cast<float>(minute) / 60.0f) * 30.0f,
          static_cast<float>(minute) * 6.0f};
}

float dayFraction(const int hour, const int minute) {
  return static_cast<float>(hour * 60 + minute) / static_cast<float>(MINUTES_PER_DAY);
}

std::string formatTime(const int hour, const int minute, const bool use12h) {
  char buf[8];
  if (use12h) {
    const int h12 = hour % 12 == 0 ? 12 : hour % 12;
    snprintf(buf, sizeof(buf), "%d:%02d", h12, minute);
  } else {
    snprintf(buf, sizeof(buf), "%02d:%02d", hour, minute);
  }
  return buf;
}

std::string formatDateCaps(const ClockTime& t) {
  return upper(weekdayName(t)) + " " + std::to_string(t.day) + " " + upper(monthName(t));
}

std::string formatDateLong(const ClockTime& t) {
  return std::string(weekdayName(t)) + ", " + std::to_string(t.day) + " " + monthName(t);
}

std::string weekdayAbbrev(const ClockTime& t) { return upper(weekdayName(t)).substr(0, 3); }

std::string monthAbbrev(const ClockTime& t) { return upper(monthName(t)).substr(0, 3); }

RefreshKind chooseRefresh(const bool firstDraw, const int minutesSinceHalf, const int intervalMinutes) {
  return firstDraw || minutesSinceHalf >= intervalMinutes ? RefreshKind::Half : RefreshKind::Fast;
}

int fullRefreshMinutes(const uint8_t settingIndex) {
  switch (settingIndex) {
    case 0:
      return 10;
    case 2:
      return 30;
    default:
      return 15;
  }
}

BatteryState batteryState(const int percent) {
  if (percent <= 5) return BatteryState::Critical;
  if (percent <= 20) return BatteryState::Low;
  return BatteryState::Normal;
}

ClockTime localFromUtc(const UtcDateTime& utc, const int utcOffsetQBiased) {
  const int64_t offsetSeconds = static_cast<int64_t>(utcOffsetQBiased - 48) * 15 * 60;
  const int64_t utcSeconds = daysFromCivil(utc.year, utc.month, utc.day) * 86400 + utc.hour * 3600 +
                             utc.minute * 60 + utc.second;
  const int64_t local = utcSeconds + offsetSeconds;
  const int64_t days = local >= 0 ? local / 86400 : (local - 86399) / 86400;
  const int64_t secondsOfDay = local - days * 86400;

  ClockTime t{};
  civilFromDays(days, t.year, t.month, t.day);
  t.weekday = static_cast<int>(((days % 7) + 11) % 7);  // 1970-01-01 was a Thursday (4)
  t.hour = static_cast<int>(secondsOfDay / 3600);
  t.minute = static_cast<int>(secondsOfDay % 3600 / 60);
  t.second = static_cast<int>(secondsOfDay % 60);
  return t;
}

}  // namespace deskclock
