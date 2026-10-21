"""Regenerate the desk clock's font headers.

Usage: python src/clock/fonts/gen_fonts.py <path to InterDisplay-Light.ttf>

Inter Display Light comes from the Inter 4.1 release
(https://github.com/rsms/inter/releases/tag/v4.1, Inter-4.1.zip,
extras/ttf/InterDisplay-Light.ttf). Inter Regular and ChareInk7 are the copies
CrossInk already ships under lib/EpdFont/builtinFonts/source. All three are
under the SIL Open Font License 1.1 (see OFL.txt here and in those folders).

Sizes are fontconvert.py points at 150 dpi: pixels per em = size * 150 / 72.
Each font keeps only the characters the clock draws.
"""
import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
CONVERT = REPO / "lib" / "EpdFont" / "scripts" / "fontconvert.py"
BUILTIN = REPO / "lib" / "EpdFont" / "builtinFonts" / "source"

DIGITS = ["--additional-intervals", "0x20,0x20", "--additional-intervals", "0x30,0x3A"]
CAPS = ["--additional-intervals", "0x20,0x20", "--additional-intervals", "0x41,0x5A"]
WORDS = ["--additional-intervals", "0x20,0x20", "--additional-intervals", "0x27,0x27",
         "--additional-intervals", "0x2D,0x2D", "--additional-intervals", "0x61,0x7A"]
ASCII = ["--additional-intervals", "0x20,0x7E"]


def fonts(inter_display_light: Path):
    inter = BUILTIN / "Inter" / "Inter-Regular.ttf"
    serif = BUILTIN / "ChareInk7" / "ChareInk7-Regular.ttf"
    serif_italic = BUILTIN / "ChareInk7" / "ChareInk7-Italic.ttf"
    # name, size, source, intervals
    return [
        # EpdFontData stores line height (advanceY) in a uint8_t, so no font may
        # need more than 255 px between lines: size 100 is the largest that fits.
        ("clock_digits_xl", 100, inter_display_light, DIGITS),  # big time, about 150 px digits
        ("clock_digits_l", 90, inter_display_light, DIGITS),  # day line time, portrait time
        ("clock_digits_m", 62, inter_display_light, DIGITS),  # dial date number
        ("clock_caps", 14, inter, CAPS),  # letter-spaced capitals
        ("clock_serif_words", 46, serif, WORDS),  # words face, regular
        ("clock_serif_words_italic", 46, serif_italic, WORDS),  # words face, italic line
        ("clock_serif_small", 13, serif, ASCII),  # words face date
    ]


def main() -> int:
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    light = Path(sys.argv[1]).resolve()
    for name, size, source, intervals in fonts(light):
        result = subprocess.run(
            [sys.executable, str(CONVERT), name, str(size), str(source), "--no-default-intervals", *intervals],
            capture_output=True, text=True, check=False)
        if result.returncode != 0:
            print(result.stderr)
            return 1
        # Keep local paths out of the committed header.
        text = re.sub(r"\* Command used: .*", f"* Command used: gen_fonts.py ({source.name}, size {size})",
                      result.stdout)
        (HERE / f"{name}.h").write_text(text, encoding="utf-8", newline="\n")
        print(f"{name}.h  {len(text):>8} chars")
    return 0


if __name__ == "__main__":
    sys.exit(main())
