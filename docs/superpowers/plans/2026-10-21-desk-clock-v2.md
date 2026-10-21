# Xteink Desk Clock Implementation Plan, v2 (CrossInk base)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans (chosen by Joel: native). Steps use checkbox (`- [ ]`) syntax.

Supersedes `Xteink Desk Clock - Plan.md` (CrossPoint base). The pure clock model, its tests and most of the clock code from that plan carry over from `D:\source\projects\crosspoint-clock` (branch `desk-clock`, commits `e288875` to `20d6f57`).

**Goal:** A desk clock mode with four selectable minimal faces, in a personal fork of CrossInk v1.6.1 for Joel's newer Xteink X4 Pro.

**Architecture:** Pure logic in `src/clock/ClockModel.*` (host-tested). `ClockActivity` draws one of four faces, light-sleeps between redraws through `src/clock/ClockSleep.*` and exits on tap, Left or Right. Time comes from the RTC through one additive `HalClock` method, converted with CrossInk's UTC offset setting. No Wi-Fi.

**Tech Stack:** C++ (gnu++2a), PlatformIO pioarduino 55.03.37, Arduino, FreeInk SDK (`699370183`), GoogleTest via CMake (zig compiler locally), `lib/EpdFont/scripts/fontconvert.py`.

**Spec:** `Xteink Desk Clock - Design.md`, with its "Revision 1" section overriding older text.

## Global Constraints

- Repo `D:\source\projects\crossink-clock`, branch `desk-clock` from CrossInk tag `v1.6.1` (`9914146e`). Remote `upstream` = `uxjulia/CrossInk`. Follow CrossInk stable release tags only.
- Build env `x4-pro` only. Local, untracked `platformio.local.ini` holds `[crossink] version = 1.6.1-clockN` and `-DCROSSINK_OTA_RELEASE_URL` for the fork. Image `.pio/build/x4-pro/firmware-x4-pro.bin`, at most 6,553,600 bytes.
- Upstream files that may be edited: `lib/hal/HalClock.{h,cpp}` (additive), `src/activities/ActivityManager.{h,cpp}`, `src/activities/home/HomeActivity.{h,cpp}`, `src/CrossPointSettings.h`, `src/SettingsList.{h,cpp}`, `lib/I18n/translations/english.yaml`, `test/CMakeLists.txt`, `CHANGELOG.md`. Everything else new, under `src/clock/` and `test/clock_model/`.
- No Wi-Fi in the clock. Time from the RTC plus `clockUtcOffsetQ`; 12/24 hour from `clockFormat`.
- 1-bit drawing; must read correctly with Night Mode (`screenInverted`) on.
- All UI strings through `tr(STR_*)`, English only. New home and title key `STR_DESK_CLOCK` = "Clock" (CrossInk's `STR_CLOCK` means "Show Clock").
- Install only through CrossInk's SD Card Firmware Update. Device: the newer unit `84:c7:bb:7b:4b:58`. Never a raw esptool write from this session.
- Toolchain venv `D:\venvs\crosspoint`, `PLATFORMIO_CORE_DIR=D:\pio\crosspoint`, builds from PowerShell. Host tests via `D:\scratch\xteink\tools\hosttest.ps1` pointed at this repo.
- No em or en dashes. No AI attribution. Explicit `git add` paths.

## Review Focus

1. The clock waking straight away from light sleep (seen on the CrossPoint build): every wake logs its cause and pin levels to the SD card, and a pin already at its wake level blocks the sleep. Owner: Task 2.
2. USB plugged or pulled while sleeping: keeps updating, still exits on tap. Owner: Task 2 hardware check.
3. Midnight and month rollover after the UTC offset is applied (UTC date differs from local date). Owner: Task 1 tests, Task 4 device check.
4. Power pressed while the clock shows: CrossInk's sleep captures the renderer orientation before the clock's onExit, so the sleep screen may come up landscape. Owner: Task 2 check; restore portrait before sleep if it does.
5. Night Mode on: faces drawn white on black must still read and refresh cleanly. Owner: Task 4 device check.

---

### Task 0: Repo, toolchain, baseline build
- [ ] Clone done (`desk-clock` at `9914146e`, submodules initialised, `*.patch eol=lf` and `.superpowers/` local only).
- [ ] Copy spec and both plans into `docs/superpowers/`. Create `platformio.local.ini` (untracked) with version `1.6.1-clock0`.
- [ ] `pio run -e x4-pro`. Expected: SUCCESS, size check passes, `1.6.1-clock0` found in the bin.
- [ ] Host tests: point `hosttest.ps1` at this repo (parameter), run; record the baseline.
- [ ] Commit docs.

### Task 1: Clock model and local time
**Files:** `src/clock/ClockModel.{h,cpp}` (from the CrossPoint fork), `test/clock_model/*`, `test/CMakeLists.txt`, `lib/hal/HalClock.{h,cpp}`.
**Interfaces (new):** `struct UtcDateTime { int year, month, day, hour, minute, second; };` and `ClockTime deskclock::localFromUtc(const UtcDateTime& utc, int utcOffsetQBiased);` (offset minutes = `(q - 48) * 15`, weekday computed from the date, 0 = Sunday). `bool HalClock::getDateTimeNow(uint16_t& y, uint8_t& mo, uint8_t& d, uint8_t& h, uint8_t& mi, uint8_t& s) const;` reads the RTC every call, false on failure or an invalid date.
- [ ] Copy the model and its 13 tests; add `add_subdirectory(clock_model)`; run, expect 13 pass.
- [ ] Failing tests for `localFromUtc`: `{2026,10,21,17,42,5}` at q=20 gives `2026-10-21 10:42:05, weekday 3`; `{2026,10,22,3,0,0}` at q=20 gives `2026-10-21 20:00` weekday 3; `{2026,11,1,6,30,0}` at q=20 gives `2026-10-31 23:30` weekday 6; `{2027,1,1,2,0,0}` at q=20 gives `2026-12-31 19:00` weekday 4; q=48 is UTC unchanged; q=68 (+5h) `{2026,12,31,21,0,0}` gives `2027-01-01 02:00` weekday 5.
- [ ] Implement with days-from-civil, run, expect pass. Add `getDateTimeNow`. Build `x4-pro`.
- [ ] Commit.

### Task 2: Clock screen, sleep, home entry, settings (hardware gate)
**Files:** `src/clock/ClockActivity.{h,cpp}`, `src/clock/ClockSleep.{h,cpp}`, `src/clock/ClockDebugLog.{h,cpp}`, ActivityManager, HomeActivity, CrossPointSettings.h, SettingsList.{h,cpp}, english.yaml.
- [ ] ClockSleep: before sleeping, if any wake pin already reads its wake level, skip the sleep and report `WakeCause::PinBusy` (the activity treats it as "try again shortly", not as an exit). After each wake, record the cause and every wake pin's level.
- [ ] ClockDebugLog: when built with `-DDESK_CLOCK_DEBUG_LOG`, append one line per event to `/.crosspoint/desk-clock.log` (open, append, close each time; cap at 64 KB by truncating).
- [ ] ClockActivity: as in the CrossPoint fork, but time via `getDateTimeNow` + `localFromUtc`; check `requestUpdateAndWait()`'s result; Left, Right or tap exits; settings read on entry.
- [ ] Home: `HomeMenuItem::CLOCK`, `goToClock()`, "Clock" goHome mapping, `HomeMenuAction::Clock` in the list and minimal builders (before Settings), raise `kCapacity` to 9, `getMenuItemCount` base count, icon `UIIcon::Recent`.
- [ ] Settings: `clockFace`, `clockOrientation`, `clockFullRefresh` fields and `SettingInfo::Enum` rows (category System), added to `buildSystemDeviceSettingsList`, `BASE_SETTINGS_CAPACITY` raised by 3; strings.
- [ ] Build (version `1.6.1-clock1`, debug log on), copy to SD, Joel installs. Checks with Joel: settings and stats still there; a book opens; Clock opens and stays; minute changes on time for an hour on battery; tap, Left, Right exit; USB plug and pull while sleeping; Power while showing, then portrait on wake. Read `/.crosspoint/desk-clock.log` over USB Drive after.
- [ ] Fix what the log shows (TDD where the logic is pure). Commit per change.

### Task 3: Fonts
- [ ] Inter Display Light (from `D:\scratch\xteink\fonts\inter-4.1`, OFL) for digits and caps; CrossInk's bundled serif (or Noto Serif) regular and italic for the words face. Subset per size, generate with `fontconvert.py`, record commands and licences in `src/clock/fonts/README.md`, `ensureFontsLoaded` with ids distinct from `fontIds.h`. Build; record size against 6,553,600.
- [ ] Commit.

### Task 4: The four faces
- [ ] `drawFace(renderer, face, orientation, ctx)` as the spec's Faces section, landscape matching the mockups, portrait sensible, low battery mark. Remove debug text.
- [ ] Install; look with Joel in Night Mode and normal, landscape and portrait; rollover check (set offset so local time is 23:58); fixes.
- [ ] Commit.

### Task 5: Battery
- [ ] Low mark at 20 percent or below; at 5 percent draw "Charge me" with the time, then CrossInk's normal deep sleep path minus the sleep screen (`display.deepSleep()`, `Storage.shutdown()`, `powerManager.startDeepSleep(gpio)` in `enterDeepSleep`'s order). Test with a forced value in an uncommitted local build. Commit.

### Task 6: Release and handover
- [ ] Turn the debug log off. Ask Joel, then fork `uxjulia/CrossInk` to `vandervalkjoel`, push `desk-clock`, release `v1.6.1-clock1` with `firmware-x4-pro.bin`; local ini OTA URL to the fork. CHANGELOG entry. `docs/desk-clock-fork.md` (taking a CrossInk release).
- [ ] Final install; 12 hour battery measurement; vault project note and Log; whole-branch review.
