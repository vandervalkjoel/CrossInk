---
type: spec
status: approved
created: 2026-10-21
---

# Xteink Desk Clock: design

A desk clock mode for Joel's Xteink X4 Pro, built into a personal fork of the CrossPoint Reader firmware. The physical stand is a separate project (session "Xteink X4 Pro desk stand (3D print)") and is not covered here.

## Intent

The X4 Pro is Joel's e-reader and stays one. Sometimes he sets it on a stand under his monitor and wants it to show the time and date at a glance, like a quiet desk clock. E-ink holds an image with no power and the clock needs no frontlight, so it should run a long time on battery between charges.

## Goals

- A "Clock" entry on the CrossPoint home screen that turns the reader into a desk clock showing time and date. (Joel)
- Four built-in minimal faces, chosen in Settings: big time, quiet dial, words, day line. (Joel: "I like all of those", choose one "in the reader")
- Landscape is the main orientation. (Joel: "mostly in landscape")
- Minimal look. (Joel: "I like minimalism")
- The fork keeps picking up new CrossPoint releases over time. (Joel)

## Revision 1 (2026-10-21): CrossInk base

This section overrides anything below that conflicts with it.

- **Base firmware is CrossInk, not CrossPoint.** Joel's device runs CrossInk (`github.com/uxjulia/CrossInk`, MIT, a CrossPoint fork with reading stats), which keeps its settings in `/.crosspoint/crossink-settings.json`. A first install of a CrossPoint 2.0 build lost his settings view; the device was restored from the 2026-10-21 flash backup and his settings came back. The fork is of CrossInk, starts at tag `v1.6.1`, and follows CrossInk's stable releases. (Joel)
- **No Wi-Fi sync in the clock.** The clock never joins Wi-Fi. Time comes from the battery-backed RTC, which CrossInk already sets over Wi-Fi (on connect, and its "Sync clock now" action). (Joel) The clock still refuses to show a time it cannot trust.
- **Time zone:** CrossInk's single `clockUtcOffsetQ` setting (quarter hours, biased by 48), no DST. Joel's is already UTC-7 (value 20), which matches British Columbia from November 2026. No Phoenix workaround, no timezone table.
- **12/24 hour:** CrossInk's existing `clockFormat` (Joel's is 12 hour).
- **Night Mode:** CrossInk inverts every screen when `screenInverted` is on (Joel's is). Faces must look right inverted (white on black).
- **Settings:** the three clock settings go in Settings, System, Device beside CrossInk's clock rows.
- **Home entry:** in CrossInk's home menu model, for the list and Dashboard themes (Joel uses Dashboard). Not added to the Cover Grid theme. (my call)
- **Fork mechanics:** `[crossink] version` and the OTA URL (`CROSSINK_OTA_RELEASE_URL`) are set in an untracked `platformio.local.ini` for local builds; build env is `x4-pro`; the firmware image is `firmware-x4-pro.bin`; app slot limit is 6,553,600 bytes.
- **Debugging light sleep:** the first CrossPoint build's clock fell out to the home screen about a second after opening: the log showed the first draw, then USB dropped (light sleep), and no wake line reached the PC. The clock writes a small debug log to the SD card so wake causes survive USB dropping.

## The device

The clock goes on Joel's newer X4 Pro (Joel), MAC 84:c7:bb:7b:4b:58, which got a full 16 MB CrossPoint image written on 2026-10-20 (`D:\scratch\xteink\restore-new-2026-10-20.log`; its stock image before that is `x4pro-NEW-stock-84c7bb7b4b58-2026-10-20.bin`). The older unit (98:c3:77:b7:d7:04) is not touched. Check the newer unit's Settings, About screen before the first install and record what it shows.

## Non-goals

- The stand, charging dock or any hardware change.
- Weather, calendar events, notifications or anything needing a network service beyond time sync.
- Upstreaming. CrossPoint's SCOPE.md rules out apps like this, so the fork stays personal.
- Other Xteink models. Only the X4 Pro (ESP32-S3, `env:x4pro`) has the battery-backed RTC this relies on.

## Anti-goals (a failure even if it technically works)

- Showing a wrong or frozen time without saying so.
- Making reading worse in any way: a regression in the reader, sleep or settings.
- A battery drain that makes the clock unusable without being plugged in.
- A fork so tangled with upstream that taking a new release becomes painful.
- Anything that can leave the device unbootable without a recovery path.

## How it behaves

### Entering and leaving

- A new "Clock" item on the home screen starts the clock activity. (Joel)
- (my call) A tap anywhere on the screen, or the Left or Right button, exits to the home screen. Power keeps its normal global meaning (handled in `main.cpp` before any activity sees it), so `main.cpp` is not edited for this. Touch always works whichever edge faces down into the stand.
- (my call) While the clock is showing, the device never auto-sleeps on the reader's inactivity timeout (`preventAutoSleep()` returns true).

### Faces

All faces are pure 1-bit black on white (no greyscale pass). Mockups were shown to Joel at 800 x 480 with this content: 10:42, Wednesday 21 October.

1. **Big time.** The time as large as it fits, centred, thin weight. The date below in small letter-spaced capitals ("WEDNESDAY 21 OCTOBER").
2. **Quiet dial.** An analog dial on the left: twelve tick marks (heavier at 12, 3, 6, 9), no numerals, an hour hand, a minute hand and a centre dot. A thin vertical rule, then on the right the date stacked: day abbreviation ("WED"), large day number ("21"), month abbreviation ("OCT").
3. **Words.** The time in words to the nearest five minutes ("twenty to eleven" at 10:42), large serif, with the hour word in italic on its own line. A short rule and the date ("Wednesday, 21 October") beneath, left aligned. Redraws only when the words change.
4. **Day line.** Big time and the date in letter-spaced capitals at upper left. Across the bottom a thin line from 00 to 24 with ticks and labels at 00, 06, 12, 18, a thicker filled segment from midnight to now, and a dot at now.

(my call) Each face also gets a portrait layout. Landscape is the one to get right; portrait should be sensible, not a showpiece.

The 12/24 hour setting that already exists applies to faces 1, 2 (no effect) and 4. Words is always words.

### Settings

(Joel: chosen in the reader's settings.) (my call) Added to the existing Settings, System, Clock screen (`src/activities/settings/ClockSettingsActivity.cpp`):

- Clock face: Big time, Quiet dial, Words, Day line. Default Big time.
- Clock orientation: Landscape, Portrait. Default Landscape. (Which landscape rotation is "up" is the builder's call, matched to how the device sits on the stand: buttons on the long edges.)
- Clock full refresh: every 10, 15, 30 minutes. Default 15.

Existing settings reused: timezone, DST mode, 12/24 hour, sync now.

### Time

- The X4 Pro has a battery-backed BM8563 RTC; the clock reads time from it.
- (my call) On entering the clock, join the last saved Wi-Fi network without UI (the pattern in `main.cpp` around lines 405 to 416), sync NTP into the RTC (`HalClock::syncFromNTP`), then turn Wi-Fi fully off. Re-sync about every 6 hours. A failed sync is not an error if the RTC is valid.
- Read the RTC fresh for each redraw. `HalClock::localTime()` caches for 10 seconds, which can show the previous minute; bypass it.
- If the RTC reports invalid time (stopped oscillator, upstream issue #3201) and no sync has succeeded, show a plain "Time not set, connect to Wi-Fi" message instead of a time. Never show a stale time.

### Timezone

(my call) British Columbia stays on UTC-7 all year from 2026; CrossPoint's "Los Angeles / Vancouver" entry (`src/util/Timezones.cpp:18`, `PST8PDT,...`) would fall back on 1 November. Set Joel's device to the existing "Phoenix" entry (`MST7`, permanent UTC-7). No timezone table edit: the table is append-only and indexed, so a fork-only entry could collide with a future upstream entry at the same index and silently change the zone.

### Refresh and sleep

- (my call) Redraw at each minute boundary (words face: only when the words change) with a FAST refresh, and a HALF refresh at the chosen full-refresh interval and on entry.
- (my call) Between redraws, use ESP32 light sleep (state kept, no reboot), timed to the next minute boundary computed from the RTC rather than a fixed 60 seconds, plus GPIO wake on the buttons and the touch controller's interrupt (GT911 INT, GPIO10) so input exits immediately. Prior art: crossmux `StandbyActivity` and `HalPowerManager::lightSleepFor`, and the Picco-only light sleep in `lib/hal/HalPowerManager.cpp:166-200`. Upstream reverted a general light-sleep idle (PR #3077) because of freezes when USB was plugged in; that case must be tested.
- Fallback if light sleep cannot be made reliable on this device: stay awake at reduced CPU between minutes (the existing idle loop), and measure the cost. Deep sleep per minute (a full reboot each minute) is the last resort.

### Battery

- (my call) Show a small battery indicator only when the battery is low (20 percent or below).
- (my call) At 5 percent or below, draw "Charge me" with the date and time of shutdown, then enter normal deep sleep, so the screen never keeps showing a frozen time as if current.
- Battery life is unknown and has not been promised. The builder measures it (percent drop over a long run on the stand) and reports it.

## The fork

- (my call) A public fork of `github.com/crosspoint-reader/crosspoint-reader` on Joel's GitHub (`vandervalkjoel`). MIT licence allows it. Creating it needs Joel's OK at the time.
- (my call) Clone to `D:\source\projects\crosspoint-clock`. Base: the 2.0 line. Upstream's latest stable is 1.6.5 (2026-09-27) and 2.0.0rc came out 2026-10-19; the files the clock touches changed heavily between them, so starting on 1.6.5 means a large merge almost at once. Build on the 2.0.0rc tag, move to the 2.0.0 tag when it ships, and from then on track stable release tags only (not RCs). If the rc shows reading problems on the device during testing, stop and tell Joel before he relies on it day to day.
- Keep the change small and mostly in new files (a new activity, new face renderers, fonts) so merges stay easy; edits to upstream files limited to the home menu, settings (including the Clock settings screen), one additive fresh-read method on `HalClock`, the OTA URL, i18n strings and the host test list.
- The OTA updater hardcodes upstream's releases URL (`src/network/OtaUpdater.cpp:22`). (my call) Point it at the fork's releases so "update online" cannot replace the clock firmware with upstream's.
- Build with PlatformIO, `env:x4pro` for development, the `x4pro-gh_release` env for anything run off USB (the dev env waits for a serial monitor at boot).
- Installing: (my call) do NOT write a fixed offset with esptool. The devices' partition tables differ from the repo's `partitions.csv` (device: app0 0x10000, app1 0x7f0000, 0x7e0000 each), and on the 2026-10-20 backup otadata selects ota_1, so a write to 0x10000 would be silently ignored and read as "my change did nothing". Install through CrossPoint's own SD card firmware update (`src/activities/settings/SdFirmwareUpdateActivity.cpp`), which writes the inactive slot of the device's real table and switches to it. Before the first install, take a fresh full 16 MB read of the unit (esptool `read-flash` with `--no-stub`, in chunks, as in `D:\scratch\xteink\backup.ps1`) and confirm afterwards that Settings, About shows the fork's version. Recovery: CrossPoint's recovery mode (Power plus Down held at boot) or writing the backup image back.
- Device settings live on the SD card under `.crosspoint/` and survive an install.

## Already known about the code

From a read-only survey of upstream at `f8936ba` (clone in `D:\scratch\xteink\src\crosspoint-reader`):

- Activities: `src/activities/Activity.h`, stack in `src/activities/ActivityManager.*`. `ClockSyncActivity.cpp` is a good minimal template.
- Home menu items are hardcoded across `ActivityManager.h:21` (enum), `HomeActivity.cpp:29-38`, `:308-333`, `:527-540`, `:574-580` and `HomeActivity.h:42-65`. A cover-grid home layout (`src/components/CoverGridHomeUi`) may have its own list; unverified.
- Strings: `lib/I18n/translations/*.yaml`, generated by `scripts/gen_i18n.py`. English only is fine.
- Drawing: `GfxRenderer` has lines with width, arcs, polygons, rects and text, but no circle and no large font. Large digits need a converted font (`lib/EpdFont/scripts/fontconvert.py`) or drawn shapes.
- Display: 800 x 480, 1 bit; FULL, HALF (about 1.7 s) and FAST (about 0.5 s) refresh; orientation via `renderer.setOrientation`, reset to portrait on exit as the reader does.
- Inputs: Left GPIO0, Right GPIO7, Power GPIO3 (active low), capacitive touch with a Home key on the GT911 (INT GPIO10).

## Left to the builder

- Fonts (thin sans for big digits, a serif for the words face), within open licences. Match the mockups' feel.
- Exact layouts, sizes and spacing; the portrait layouts.
- Code structure inside the new activity and how faces are organised.
- The home menu icon.
- How fork releases are produced for OTA (the repo's own release workflow in GitHub Actions, or manual).
- The order of work, except that light sleep on the real device, including with USB plugged in, is tested early, before the faces are polished.
