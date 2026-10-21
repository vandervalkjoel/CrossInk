# Xteink Desk Clock Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a desk clock mode with four selectable minimal faces to a personal fork of CrossPoint Reader 2.0 for Joel's newer Xteink X4 Pro.

**Architecture:** All clock logic that can be pure (words, rounding, wake timing, hand angles, date text, refresh, battery and sync policy) lives in `src/clock/ClockModel.*` with no Arduino dependency and is covered by host gtest suites. A new `ClockActivity` draws one of four face renderers, light-sleeps between redraws through `src/clock/ClockSleep.*`, and exits on tap, Left or Right. Upstream files get small, listed edits only, so new CrossPoint releases merge cleanly.

**Tech Stack:** C++ (gnu++2a), PlatformIO with the pioarduino ESP32 platform, Arduino framework, FreeInk SDK, GoogleTest via CMake for host tests, `lib/EpdFont/scripts/fontconvert.py` for fonts.

**Spec:** `C:\Users\vande\Obsidian\Development\01-projects\Xteink Desk Clock - Design.md` (copied into the repo as `docs/superpowers/specs/2026-10-21-desk-clock-design.md` in Task 0). Read it before any task.

## Global Constraints

- Base: upstream tag `2.0.0rc` (`91ed3ef8a48962e0066a22b149cab3dfa0a0ee05`). Move to `2.0.0` when it ships; then stable tags only.
- Targets: `env:x4pro` for development, `env:x4pro-gh_release` for anything run off USB. No other board.
- Upstream files that may be edited, and nothing else: `src/activities/ActivityManager.h`, `src/activities/ActivityManager.cpp`, `src/activities/home/HomeActivity.h`, `src/activities/home/HomeActivity.cpp`, `src/CrossPointSettings.h`, `src/SettingsList.cpp`, `src/activities/settings/ClockSettingsActivity.h`, `src/activities/settings/ClockSettingsActivity.cpp`, `lib/hal/HalClock.h`, `lib/hal/HalClock.cpp` (additive only), `src/network/OtaUpdater.cpp` (URL only), `lib/I18n/translations/english.yaml`, `test/CMakeLists.txt`, `platformio.ini` (version string only). Everything else is new, under `src/clock/` and `test/clock_model/`.
- 1-bit black on white only. No grayscale render pass.
- Install on the device only through CrossPoint's SD card firmware update. Never write a fixed flash offset with esptool. Before the first install, a fresh full 16 MB backup of the unit.
- Device: the newer X4 Pro, MAC `84:c7:bb:7b:4b:58`. Never touch the older unit (`98:c3:77:b7:d7:04`).
- Toolchain lives in a venv at `D:\venvs\crosspoint` with `PLATFORMIO_CORE_DIR=D:\pio\crosspoint`. Never `pip install` into the global Python 3.12 (ESPHome depends on its pins) and never use the default `~/.platformio`.
- Run every PlatformIO build from PowerShell, never Git Bash (ESP-IDF under MSYS silently skips recompiling and still reports success).
- Strings shown on the device (English only): `Clock` (existing `STR_CLOCK`), `Clock face`, `Big time`, `Quiet dial`, `Words`, `Day line`, `Clock orientation`, `Landscape`, `Portrait`, `Clock full refresh`, `Every 10 min`, `Every 15 min`, `Every 30 min`, `Time not set, connect to Wi-Fi`, `Charge me`.
- No em dashes or en dashes in any file or string. No AI attribution in commits. Stage explicit paths only (never `git add -A` or `git add .`).
- Every step that needs the physical device is done by Joel or with Joel present: ask before it, and say what he will see.

## Review Focus

1. USB cable plugged in or pulled while the clock is light-sleeping: the clock keeps updating and still exits on a tap (upstream reverted light sleep once for USB-attach freezes). Owner: Task 3, Step 7.
2. Wi-Fi unavailable, wrong password or no saved network when the clock starts or re-syncs: the clock shows RTC time within 15 s and input still works; no hang, no reboot loop. Owner: Task 7, Step 6.
3. Midnight and month rollover while showing: the date text and the day line reset at 00:00, and the words face reads "twelve o'clock". Owner: Task 1 tests (rollover cases) and Task 6, Step 5.
4. A Power press while the clock is showing (Joel's short-power setting, which may be sleep): the device sleeps normally and wakes with the reader in portrait, not stuck in the clock's landscape. Owner: Task 3, Step 7.
5. Entering and leaving the clock twenty times in a row: no crash, no growing heap, fonts not inserted twice. Owner: Task 4, Step 6.

---

### Task 0: Fork, toolchain and a known-good install path

**Files:**
- Create: `docs/superpowers/specs/2026-10-21-desk-clock-design.md` (copy of the vault spec)
- Create: `docs/superpowers/plans/2026-10-21-desk-clock.md` (copy of this plan)
- Modify: `platformio.ini` (version string only)

- [ ] **Step 1: Ask Joel to approve creating the public fork** `vandervalkjoel/crosspoint-reader` with `gh repo fork crosspoint-reader/crosspoint-reader --clone=false`. Do not run it before he says yes.
- [ ] **Step 2: Clone and branch.** `git clone https://github.com/vandervalkjoel/crosspoint-reader.git D:\source\projects\crosspoint-clock`, `git -C D:\source\projects\crosspoint-clock remote add upstream https://github.com/crosspoint-reader/crosspoint-reader.git`, fetch tags, `git checkout -b desk-clock 2.0.0rc`, `git submodule update --init --recursive`.
- [ ] **Step 3: Toolchain.** Create `D:\venvs\crosspoint` (Python 3.12 is acceptable if the upstream `pio` pin installs; the project asks for 3.13, so prefer installing 3.13 into that venv's base only if 3.12 fails). Install `platformio==6.1.19`, `cmake`, `ninja`, `ziglang`, and `lib/EpdFont/scripts/requirements.txt` into it. Set `PLATFORMIO_CORE_DIR=D:\pio\crosspoint` for every build command.
- [ ] **Step 4: Host tests build.** Configure with `cmake -S test -B build/test -G Ninja -DCMAKE_C_COMPILER="python -m ziglang cc" -DCMAKE_CXX_COMPILER="python -m ziglang c++"` (wrap the two in small `.cmd` shims in `D:\venvs\crosspoint\Scripts` if CMake refuses arguments in the compiler path). Run `ctest --test-dir build/test --output-on-failure`. Expected: every upstream suite passes. If zig cannot build GoogleTest, fall back to w64devkit unzipped under `D:\tools` and say so.
- [ ] **Step 5: Device build.** In `platformio.ini`, change the `x4pro-gh_release` version string to end in `-clock0`. Run `pio run -e x4pro-gh_release`. Expected: `SUCCESS` and `.pio\build\x4pro-gh_release\firmware.bin`. Confirm the build really ran by finding `-clock0` in the bin with Python (`open(p,'rb').read().count(b'-clock0')`), not with `strings`.
- [ ] **Step 6: Backup and install, with Joel.** Ask Joel to read Settings, About on the newer unit and note it in the vault project note. Take a full 16 MB backup with `esptool --no-stub read-flash` in 512 KB chunks (pattern: `D:\scratch\xteink\backup.ps1`) to `D:\scratch\xteink\x4pro-new-pre-clock-<date>.bin` and check its MAC is `84:c7:bb:7b:4b:58`. Copy the `-clock0` firmware.bin to the SD card, install through CrossPoint's SD card firmware update. Expected: About shows the `-clock0` version, a book opens and pages turn.
- [ ] **Step 7: Commit the docs and version change.**

```
git add docs/superpowers/specs/2026-10-21-desk-clock-design.md docs/superpowers/plans/2026-10-21-desk-clock.md platformio.ini
git commit -m "docs: desk clock design and plan; mark fork version"
```

### Task 1: Pure clock model

**Files:**
- Create: `src/clock/ClockModel.h`, `src/clock/ClockModel.cpp`
- Create: `test/clock_model/CMakeLists.txt`, `test/clock_model/ClockModelTest.cpp`
- Modify: `test/CMakeLists.txt` (add `add_subdirectory(clock_model)`)

**Interfaces:**
- Produces (namespace `deskclock`, no Arduino includes; `ClockModel.h` may include `<ctime>`, `<cstdint>`, `<string>`):
  - `struct ClockTime { int year; int month; int day; int weekday; int hour; int minute; int second; };` (month 1-12, weekday 0 = Sunday)
  - `ClockTime fromTm(const std::tm& t);`
  - `enum class ClockFace : uint8_t { BigTime = 0, QuietDial = 1, Words = 2, DayLine = 3, Count = 4 };`
  - `enum class ClockOrientation : uint8_t { Landscape = 0, Portrait = 1, Count = 2 };`
  - `struct WordsTime { std::string line1; std::string line2; };` (line2 is drawn italic)
  - `WordsTime wordsForTime(int hour, int minute);`
  - `int wordsBucket(int hour, int minute);` (0..287, changes when the words change)
  - `int secondsUntilNextRedraw(const ClockTime& t, ClockFace face);` (1..300)
  - `struct HandAngles { float hourDeg; float minuteDeg; };` and `HandAngles handAngles(int hour, int minute);` (degrees clockwise from 12)
  - `float dayFraction(int hour, int minute);`
  - `std::string formatTime(int hour, int minute, bool use12h);`
  - `std::string formatDateCaps(const ClockTime& t);`, `std::string formatDateLong(const ClockTime& t);`, `std::string weekdayAbbrev(const ClockTime& t);`, `std::string monthAbbrev(const ClockTime& t);`
  - `enum class RefreshKind { Fast, Half };` and `RefreshKind chooseRefresh(bool firstDraw, int minutesSinceHalf, int intervalMinutes);`
  - `int fullRefreshMinutes(uint8_t settingIndex);` (0 = 10, 1 = 15, 2 = 30, anything else = 15)
  - `enum class BatteryState { Normal, Low, Critical };` and `BatteryState batteryState(int percent);`
  - `bool syncDue(bool hasSynced, int64_t lastSyncUtc, int64_t nowUtc);` (6 hours)

- [ ] **Step 1: Write the failing tests** in `test/clock_model/ClockModelTest.cpp` (CMake target `ClockModelTest`, built like `test/home_button/CMakeLists.txt`, compiling `../../src/clock/ClockModel.cpp` with `../../src` on the include path):

```cpp
using namespace deskclock;
TEST(Words, NearestFive) {
  EXPECT_EQ(wordsForTime(10, 42).line1, "twenty to");   EXPECT_EQ(wordsForTime(10, 42).line2, "eleven");
  EXPECT_EQ(wordsForTime(10, 43).line1, "quarter to");
  EXPECT_EQ(wordsForTime(10, 32).line1, "half past");    EXPECT_EQ(wordsForTime(10, 32).line2, "ten");
  EXPECT_EQ(wordsForTime(10, 57).line1, "five to");      EXPECT_EQ(wordsForTime(10, 57).line2, "eleven");
  EXPECT_EQ(wordsForTime(10, 58).line1, "eleven");       EXPECT_EQ(wordsForTime(10, 58).line2, "o'clock");
  EXPECT_EQ(wordsForTime(10, 23).line1, "twenty-five past");
  EXPECT_EQ(wordsForTime(10, 37).line1, "twenty-five to");
  EXPECT_EQ(wordsForTime(13, 15).line1, "quarter past"); EXPECT_EQ(wordsForTime(13, 15).line2, "one");
  EXPECT_EQ(wordsForTime(12, 30).line2, "twelve");
}
TEST(Words, Midnight) {
  EXPECT_EQ(wordsForTime(23, 58).line1, "twelve"); EXPECT_EQ(wordsForTime(23, 58).line2, "o'clock");
  EXPECT_EQ(wordsForTime(0, 0).line1, "twelve");
}
TEST(Words, Bucket) {
  EXPECT_EQ(wordsBucket(10, 42), wordsBucket(10, 38));
  EXPECT_NE(wordsBucket(10, 42), wordsBucket(10, 43));
  EXPECT_EQ(wordsBucket(23, 58), wordsBucket(0, 2));
}
TEST(Redraw, MinuteFaces) {
  EXPECT_EQ(secondsUntilNextRedraw({2026,10,21,3,10,42,10}, ClockFace::BigTime), 50);
  EXPECT_EQ(secondsUntilNextRedraw({2026,10,21,3,10,42,59}, ClockFace::DayLine), 1);
  EXPECT_EQ(secondsUntilNextRedraw({2026,10,21,3,10,42,0}, ClockFace::QuietDial), 60);
  EXPECT_EQ(secondsUntilNextRedraw({2026,10,21,3,23,59,30}, ClockFace::BigTime), 30);
}
TEST(Redraw, WordsFace) {
  EXPECT_EQ(secondsUntilNextRedraw({2026,10,21,3,10,42,10}, ClockFace::Words), 50);
  EXPECT_EQ(secondsUntilNextRedraw({2026,10,21,3,10,43,0}, ClockFace::Words), 300);
}
TEST(Dial, Angles) {
  EXPECT_FLOAT_EQ(handAngles(10, 42).hourDeg, 321.0f); EXPECT_FLOAT_EQ(handAngles(10, 42).minuteDeg, 252.0f);
  EXPECT_FLOAT_EQ(handAngles(0, 0).hourDeg, 0.0f);
  EXPECT_FLOAT_EQ(handAngles(15, 30).hourDeg, 105.0f); EXPECT_FLOAT_EQ(handAngles(15, 30).minuteDeg, 180.0f);
}
TEST(DayLine, Fraction) {
  EXPECT_FLOAT_EQ(dayFraction(0, 0), 0.0f);
  EXPECT_NEAR(dayFraction(10, 42), 642.0f / 1440.0f, 1e-6);
  EXPECT_NEAR(dayFraction(23, 59), 1439.0f / 1440.0f, 1e-6);
}
TEST(Text, Time) {
  EXPECT_EQ(formatTime(10, 42, false), "10:42"); EXPECT_EQ(formatTime(9, 5, false), "09:05");
  EXPECT_EQ(formatTime(0, 7, false), "00:07");   EXPECT_EQ(formatTime(9, 5, true), "9:05");
  EXPECT_EQ(formatTime(0, 7, true), "12:07");    EXPECT_EQ(formatTime(13, 0, true), "1:00");
}
TEST(Text, Date) {
  const ClockTime t{2026, 10, 21, 3, 10, 42, 0};
  EXPECT_EQ(formatDateCaps(t), "WEDNESDAY 21 OCTOBER");
  EXPECT_EQ(formatDateLong(t), "Wednesday, 21 October");
  EXPECT_EQ(weekdayAbbrev(t), "WED"); EXPECT_EQ(monthAbbrev(t), "OCT");
  EXPECT_EQ(formatDateCaps({2026, 11, 1, 0, 0, 0, 0}), "SUNDAY 1 NOVEMBER");
}
TEST(Text, FromTm) {
  std::tm tm{}; tm.tm_year = 126; tm.tm_mon = 9; tm.tm_mday = 21; tm.tm_wday = 3; tm.tm_hour = 10; tm.tm_min = 42; tm.tm_sec = 5;
  const ClockTime t = fromTm(tm);
  EXPECT_EQ(t.year, 2026); EXPECT_EQ(t.month, 10); EXPECT_EQ(t.day, 21); EXPECT_EQ(t.second, 5);
}
TEST(Policy, Refresh) {
  EXPECT_EQ(chooseRefresh(true, 0, 15), RefreshKind::Half);
  EXPECT_EQ(chooseRefresh(false, 14, 15), RefreshKind::Fast);
  EXPECT_EQ(chooseRefresh(false, 15, 15), RefreshKind::Half);
  EXPECT_EQ(fullRefreshMinutes(0), 10); EXPECT_EQ(fullRefreshMinutes(1), 15);
  EXPECT_EQ(fullRefreshMinutes(2), 30); EXPECT_EQ(fullRefreshMinutes(9), 15);
}
TEST(Policy, Battery) {
  EXPECT_EQ(batteryState(21), BatteryState::Normal); EXPECT_EQ(batteryState(20), BatteryState::Low);
  EXPECT_EQ(batteryState(6), BatteryState::Low);     EXPECT_EQ(batteryState(5), BatteryState::Critical);
  EXPECT_EQ(batteryState(0), BatteryState::Critical);
}
TEST(Policy, Sync) {
  const int64_t last = 1'800'000'000;
  EXPECT_TRUE(syncDue(false, 0, last));
  EXPECT_FALSE(syncDue(true, last, last + 6 * 3600 - 1));
  EXPECT_TRUE(syncDue(true, last, last + 6 * 3600));
  EXPECT_TRUE(syncDue(true, last, last - 60));  // clock moved backwards
}
```

- [ ] **Step 2: Run it to see it fail.** `cmake --build build/test --target ClockModelTest`. Expected: build fails, `ClockModel.h` not found.
- [ ] **Step 3: Implement `src/clock/ClockModel.cpp`.** Words round with `(minute + 2) / 5 * 5`; 60 rolls to the next hour. Hour words are twelve-hour (`one` .. `twelve`). On the hour the result is `{hourWord, "o'clock"}`; otherwise `{phrase, hourWord}` with phrases `five past, ten past, quarter past, twenty past, twenty-five past, half past, twenty-five to, twenty to, quarter to, ten to, five to`, and the `to` phrases name the next hour. `wordsBucket` is `((hour * 60 + minute + 2) / 5) % 288`. For the words face, the next redraw is the next minute whose bucket differs, counted in whole seconds from `t`. Day and month names in English.
- [ ] **Step 4: Run the tests.** `ctest --test-dir build/test -R ClockModel --output-on-failure`. Expected: all `ClockModelTest` cases pass.
- [ ] **Step 5: Commit** (`src/clock/ClockModel.h src/clock/ClockModel.cpp test/clock_model/CMakeLists.txt test/clock_model/ClockModelTest.cpp test/CMakeLists.txt`, message `feat(clock): pure clock model with host tests`).

### Task 2: Fresh RTC read

**Files:**
- Modify: `lib/hal/HalClock.h`, `lib/hal/HalClock.cpp` (additive)

**Interfaces:**
- Produces: `bool HalClock::localTimeNow(struct tm& out) const;` reads the RTC on every call, applies the TZ rule, refreshes the cache, and returns false when the RTC is absent or `Rtc::now` fails (never falls back to a cached value).

- [ ] **Step 1: Implement `localTimeNow`** next to `localTime`, reusing `epochFromUtc`.
- [ ] **Step 2: Build.** `pio run -e x4pro`. Expected: `SUCCESS`.
- [ ] **Step 3: Commit** (`lib/hal/HalClock.h lib/hal/HalClock.cpp`, `feat(hal): uncached RTC read for the desk clock`).

### Task 3: Clock skeleton, home entry and light sleep (hardware gate)

The riskiest part goes first. The face here is plain `UI_12_FONT_ID` text; real faces come in Tasks 5 and 6.

**Files:**
- Create: `src/clock/ClockActivity.h`, `src/clock/ClockActivity.cpp`, `src/clock/ClockSleep.h`, `src/clock/ClockSleep.cpp`
- Modify: `src/activities/ActivityManager.h` (add `CLOCK` to `HomeMenuItem`, declare `void goToClock();`), `src/activities/ActivityManager.cpp` (define it like `goToFileTransfer`), `src/activities/home/HomeActivity.h` and `HomeActivity.cpp` (menu count, index converters, label `tr(STR_CLOCK)`, an existing icon, activation)

**Interfaces:**
- Consumes: Task 1 model, Task 2 `localTimeNow`.
- Produces:
  - `enum class WakeCause { Timer, Input };` and `WakeCause deskclock::lightSleepFor(uint32_t ms);` arms a timer wake plus GPIO level wakes on Left (GPIO0), Right (GPIO7), Power (GPIO3) (all active low) and GT911 INT (GPIO10, active low), runs `esp_light_sleep_start()`, then restores each pin exactly as `HalPowerManager::holdWhileUsbPowered` does (`gpio_wakeup_disable`, `gpio_set_intr_type(..., GPIO_INTR_DISABLE)`, `esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL)`). Returns `Input` when a GPIO woke it. The power latch and rail enables stay held.
  - `class ClockActivity : public Activity` with `onEnter`, `onExit`, `loop`, `render`, `preventAutoSleep()` returning true.

- [ ] **Step 1: Implement `ClockSleep`.** Take pins from `BoardConfig::ACTIVE` where it names them, else the constants above.
- [ ] **Step 2: Implement `ClockActivity`.** `onEnter`: remember the renderer orientation, set `LandscapeClockwise` (Task 4 makes it a setting), draw. `loop`: Left, Right (`mappedInput.wasPressed`) or a tap (`wasScreenTapped`) exits through `onGoHome()`. When the minute changed, redraw with `requestUpdateAndWait()`. Then, if no input is pending, `lightSleepFor(secondsUntilNextRedraw(...) * 1000 + 500)`; an `Input` wake exits. If the RTC read fails, draw `Time not set, connect to Wi-Fi`. Draw the battery percent in a corner (debug only, removed in Task 8). `onExit`: restore orientation to portrait. Log each wake with cause and RTC time (`LOG_INF("CLOCK", ...)`).
- [ ] **Step 3: Wire the home entry.** Follow the existing pattern for `FILE_TRANSFER` in every place it appears.
- [ ] **Step 4: Build.** `pio run -e x4pro-gh_release` (bump the version suffix to `-clock1`). Expected: `SUCCESS`, suffix found in the bin.
- [ ] **Step 5: Install with Joel** through the SD card update.
- [ ] **Step 6: One-hour run on battery, with Joel.** Clock shows; the minute changes on time for 60 minutes; a tap, Left and Right each exit to home. Record the battery percent at start and end.
- [ ] **Step 7: Edge checks, with Joel** (Review Focus 1 and 4). Plug USB in while the clock is sleeping, wait three minutes, then unplug: the minute keeps updating and a tap exits. Press Power while the clock shows: the device does what Joel's short-power setting says, and after waking a book opens in portrait.
- [ ] **Step 8: Gate.** If Step 6 or 7 fails and cannot be fixed in `ClockSleep`, switch `loop` to the fallback in the spec (stay awake at reduced CPU, polling input) and measure again. Stop and tell Joel before going on with a fallback.
- [ ] **Step 9: Commit** (the files above, `feat(clock): clock activity with light sleep and home entry`).

### Task 4: Settings

**Files:**
- Modify: `src/CrossPointSettings.h` (clock block: `uint8_t clockFace = 0; uint8_t clockOrientation = 0; uint8_t clockFullRefresh = 1;`), `src/SettingsList.cpp` (one `SettingInfo::Enum` row each, keys `clockFace`, `clockOrientation`, `clockFullRefresh`), `src/activities/settings/ClockSettingsActivity.h` and `.cpp` (three rows before `ITEM_SYNC`, each tap cycles the value, then `SETTINGS.saveToFile()`), `lib/I18n/translations/english.yaml` (new keys for the Global Constraints strings, English only; the generator fills other languages from English)
- Modify: `src/clock/ClockActivity.cpp` (read the three settings)

- [ ] **Step 1: Add the fields, rows and strings.** Out-of-range stored values read as the defaults (`ClockFace::BigTime`, `Landscape`, `fullRefreshMinutes`).
- [ ] **Step 2: Use them in `ClockActivity`.** Orientation `Landscape` is `LandscapeClockwise`, `Portrait` is `Portrait`; if the stand shows the screen upside down, Joel says so in Step 5 and it becomes `LandscapeCounterClockwise`. HALF refresh by `chooseRefresh` with `fullRefreshMinutes(SETTINGS.clockFullRefresh)`.
- [ ] **Step 3: Host tests still pass.** `ctest --test-dir build/test --output-on-failure`. Expected: all pass.
- [ ] **Step 4: Build** `x4pro-gh_release` (`-clock2`) and install with Joel.
- [ ] **Step 5: Check, with Joel.** Each new setting cycles through its values and survives a restart. The clock opens in the chosen orientation the right way up on the stand.
- [ ] **Step 6: Enter and leave twenty times** (Review Focus 5). Log `ESP.getFreeHeap()` on each `onEnter`. Expected: no crash and free heap within 2 KB of the first reading by the twentieth entry.
- [ ] **Step 7: Commit** (`feat(clock): face, orientation and refresh settings`).

### Task 5: Fonts

**Files:**
- Create: `src/clock/fonts/` (generated headers), `src/clock/ClockFonts.h`, `src/clock/ClockFonts.cpp`, `src/clock/fonts/README.md` (source font, licence, exact `fontconvert.py` command per size)

**Interfaces:**
- Produces: `void deskclock::ensureFontsLoaded(GfxRenderer& r);` (inserts once per boot, safe to call on every `onEnter`) and font id constants `CLOCK_DIGITS_XL_FONT_ID` (big time, about 220 px digit height in landscape), `CLOCK_DIGITS_L_FONT_ID` (day line, about 180 px), `CLOCK_DIGITS_M_FONT_ID` (dial date number, about 130 px), `CLOCK_CAPS_FONT_ID` (letter-spaced capitals, about 28 px), `CLOCK_SERIF_FONT_ID` regular and italic (about 96 px), `CLOCK_SERIF_SMALL_FONT_ID` (about 26 px). Ids are fixed non-zero constants distinct from those in `src/fontIds.h`.

- [ ] **Step 1: Pick the fonts** (builder's choice, open licence: a thin sans such as Inter Light, a serif such as Source Serif 4). Subset each size to the glyphs it needs: digits and colon for the digit sizes; A-Z and space for caps; lowercase, hyphen, apostrophe, comma, digits and capitals for the serif.
- [ ] **Step 2: Generate** with `lib/EpdFont/scripts/fontconvert.py` from the venv; record each command in the README.
- [ ] **Step 3: Implement `ensureFontsLoaded`** with `renderer.insertFont`, guarded by a static flag.
- [ ] **Step 4: Build** `x4pro-gh_release`. Expected: `SUCCESS`. Record the firmware size before and after; it must still fit the device's 0x7e0000 app slot.
- [ ] **Step 5: Commit** (`feat(clock): clock fonts`).

### Task 6: The four faces

**Files:**
- Create: `src/clock/ClockFaces.h`, `src/clock/ClockFaces.cpp`
- Modify: `src/clock/ClockActivity.cpp` (call `drawFace`; remove the debug battery text)

**Interfaces:**
- Consumes: Task 1 model, Task 5 fonts.
- Produces: `struct FaceContext { ClockTime time; bool use12h; bool lowBattery; int batteryPercent; };` and `void deskclock::drawFace(GfxRenderer& r, ClockFace face, ClockOrientation o, const FaceContext& ctx);` drawing into a cleared buffer for the current orientation.

- [ ] **Step 1: Implement the faces as specified in the spec's Faces section**, landscape first, matching the mockups (800 x 480). Quiet dial: ticks with `drawLine` at width 6 (12, 3, 6, 9) and 3 (others), hands at widths 10 and 5 with `handAngles`, a filled centre dot from `fillPolygon`. Day line: 00 to 24 line with ticks and labels at 00, 06, 12, 18, filled segment to `dayFraction`, dot at now. Letter spacing by drawing caps one glyph at a time. When `lowBattery`, draw a small battery outline with the percent in the bottom right corner on every face.
- [ ] **Step 2: Portrait layouts** for all four (480 x 800), sensible, not polished.
- [ ] **Step 3: Build and install** (`-clock3`).
- [ ] **Step 4: Look with Joel.** Each face in landscape, then portrait. Use CrossPoint's screenshot feature to capture each and put the images in the vault note beside the mockups. Joel's comments become fixes in this task.
- [ ] **Step 5: Rollover** (Review Focus 3). Set the timezone so local time is 23:58, watch through 00:00: date text changes, the day line resets, words reads `twelve o'clock`. Put the timezone back to Phoenix after.
- [ ] **Step 6: Commit** (`feat(clock): big time, quiet dial, words and day line faces`).

### Task 7: Time sync

**Files:**
- Create: `src/clock/ClockWifiSync.h`, `src/clock/ClockWifiSync.cpp`
- Modify: `src/clock/ClockActivity.cpp`

**Interfaces:**
- Produces: `enum class SyncResult { Ok, NoNetwork, NoConnect, NtpFailed };` and `SyncResult deskclock::syncTimeOnce(uint32_t connectTimeoutMs);` joins the last saved network without UI (the pattern in `main.cpp`'s sleep-time Wi-Fi join, using `WIFI_STORE`), calls `halClock.syncFromNTP()`, then `WiFi.disconnect(true)` and `WiFi.mode(WIFI_OFF)` on every path. Sets `SETTINGS.clockHasBeenSynced = 1` and saves only when it changes.

- [ ] **Step 1: Implement `syncTimeOnce`** with a 10000 ms connect timeout.
- [ ] **Step 2: Entry.** If the RTC read is valid, draw first, then sync and redraw if the minute moved. If it is not valid, draw `Time not set, connect to Wi-Fi`, sync, then draw the face if the sync worked.
- [ ] **Step 3: Re-sync** when `syncDue(...)` says so (6 hours), measured in RTC UTC seconds kept in the activity. Light sleep never runs past the next sync time.
- [ ] **Step 4: Exit.** If Wi-Fi was used during this clock session, call `silentRestart()` from `onExit` after restoring orientation, as `ClockSyncActivity` does, to give the reader a clean heap.
- [ ] **Step 5: Build and install** (`-clock4`).
- [ ] **Step 6: Failure checks, with Joel** (Review Focus 2). With the router's Wi-Fi off, and again with no saved network: the clock shows RTC time within 15 s of entry and a tap exits. Expected: no reboot loop, and log lines show `NoConnect` or `NoNetwork`.
- [ ] **Step 7: Commit** (`feat(clock): wifi time sync on entry and every six hours`).

### Task 8: Battery

**Files:**
- Modify: `src/clock/ClockActivity.cpp`, `src/clock/ClockSleep.h`, `src/clock/ClockSleep.cpp`

**Interfaces:**
- Produces: `[[noreturn]] void deskclock::shutdownForLowBattery();` turns Wi-Fi off, calls `trustedtime::note()`, `display.deepSleep()`, `Storage.prepareForDeepSleep()` and `powerManager.startDeepSleep(gpio)`, in the order `enterDeepSleep` in `main.cpp` uses, without drawing the sleep screen.

- [ ] **Step 1: Low.** Read `powerManager.getBatteryPercentage()` at each redraw; `batteryState` Low sets `FaceContext::lowBattery`.
- [ ] **Step 2: Critical.** Draw `Charge me` with the date and time of shutdown (UI font, centred), HALF refresh, wait for it, then `shutdownForLowBattery()`.
- [ ] **Step 3: Build and install** (`-clock5`). Test the critical path by temporarily forcing the percent to 5 in a local build that is never committed; Joel confirms the screen and that Power wakes the device to a normal boot.
- [ ] **Step 4: Commit** (`feat(clock): low battery mark and safe shutdown`).

### Task 9: Release path and handover

**Files:**
- Modify: `src/network/OtaUpdater.cpp` (releases URL to `https://api.github.com/repos/vandervalkjoel/crosspoint-reader/releases/latest`, keeping upstream's exact URL form)
- Create: `docs/desk-clock-fork.md` (how to take a new upstream release: fetch the tag, merge into `desk-clock`, the list of upstream files touched, rebuild, run host tests, install by SD card)

- [ ] **Step 1: Change the OTA URL.** Build. Expected: `SUCCESS`, and the new URL found in the bin.
- [ ] **Step 2: Release.** Ask Joel before pushing. Push `desk-clock`, tag `2.0.0rc-clock1`, and publish a GitHub release on the fork with `firmware-x4pro.bin` attached (manually with `gh release create` unless the repo's release workflow runs cleanly on the fork).
- [ ] **Step 3: Final install with Joel**, set the timezone to Phoenix, choose a face.
- [ ] **Step 4: Battery measurement.** Joel leaves it on the stand on battery for at least 12 hours; record start and end percent and the time, and work out percent per day. Report the number; do not round it into a promise.
- [ ] **Step 5: Vault.** Update the project note `01-projects/Xteink Desk Clock.md` (create from `meta/templates/project.md`): status, the measured battery figure, the fork URL, a dated Log entry. Add an Accomplishment Bank bullet if it shipped.
- [ ] **Step 6: Commit** (`src/network/OtaUpdater.cpp docs/desk-clock-fork.md`, `chore(clock): point online updates at the fork; fork maintenance notes`).
