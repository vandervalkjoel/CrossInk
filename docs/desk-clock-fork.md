# Desk clock fork: maintenance notes

This fork (`vandervalkjoel/CrossInk`, branch `desk-clock`) is CrossInk plus a desk clock for the Xteink X4 Pro. The design and plans are in `docs/superpowers/`.

## What the fork changes

New files: everything under `src/clock/` and `test/clock_model/`, `docs/desk-clock-fork.md`, `docs/superpowers/`.

Edits to CrossInk's own files, kept small so releases merge cleanly:

- `lib/hal/HalClock.{h,cpp}`: one added method, `getDateTimeNow` (uncached RTC read).
- `src/activities/ActivityManager.{h,cpp}`: `HomeMenuItem::CLOCK`, `goToClock()`, the "Clock" name mapping in `goHome`.
- `src/activities/home/HomeActivity.{h,cpp}`: `HomeMenuAction::Clock` in the list and minimal/Dashboard menus, menu capacity 9, `onClockOpen()`.
- `src/CrossPointSettings.h`, `src/SettingsList.{h,cpp}`: `clockFace`, `clockOrientation`, `clockFullRefresh` and their rows under System > Device; `BASE_SETTINGS_CAPACITY` raised by 3.
- `lib/I18n/translations/english.yaml`: the clock's strings.
- `test/CMakeLists.txt`: `add_subdirectory(clock_model)`.
- `platformio.ini`: `CROSSINK_OTA_RELEASE_URL` for `env:x4-pro`.
- `CHANGELOG.md`.

## Taking a new CrossInk release

```
git fetch upstream --tags
git checkout desk-clock
git merge v<new version>
```

Resolve conflicts in the files listed above, then set `[crossink] version = <new version>-clock1` in your local `platformio.local.ini` (untracked), build `pio run -e x4-pro`, run the clock tests, install, and release (below).

## Building on Windows

CrossInk is built on Linux in its own CI. On Windows two local, uncommitted tweaks are needed:

- `scripts/git_branch.py`: skip `AddBuildMiddleware` when `os.name == 'nt'` (it collides with the platform's long-path workaround: "Two environments with different actions were specified for the same target"). Kept out of commits with `git update-index --skip-worktree scripts/git_branch.py`.
- `.git/info/attributes`: `*.patch eol=lf`, or the SdFat and JPEGDEC patches check out as CRLF and refuse to apply.

Host tests: `test/clock_model` builds with any C++20 compiler; on this machine `D:\scratch\xteink\tools\hosttest.ps1` builds just that suite with zig.

## Debugging the clock on the device

USB serial drops while the clock light-sleeps. A local, untracked `src/clock/ClockDebugLocal.h` containing `#define DESK_CLOCK_DEBUG_LOG` makes the clock write `/.crosspoint/desk-clock.log` on the SD card (one line per open, wake and close). Adding `#define DESK_CLOCK_WAKE_SWEEP` makes each open test every wake pin alone and log which ones end a sleep early; that sweep is how the X4 Pro's touch line was found to pulse every few milliseconds, which is why touch does not wake the clock.

## Releasing

Build with the version set, then publish `firmware-x4-pro.bin` on a GitHub release of this fork. CrossInk's online updater compares versions up to the first `-`, so a clock-only change on the same CrossInk version (1.6.1-clock2 after 1.6.1-clock1) is not offered as an update; install those by SD card or USB.

## Known issues

- After the clock has light-slept with USB plugged in, the USB serial link can stop answering until the cable is replugged.
