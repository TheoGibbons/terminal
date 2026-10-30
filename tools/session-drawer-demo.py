#!/usr/bin/env python3
"""Exercise live bottom-row previews in Windows Terminal or WSL.

Run in several tabs with different --label values. Ctrl+C restores the screen.
This emits real VT output and never changes the terminal dimensions.
"""

import argparse
import os
import shutil
import sys
import textwrap
import time


def wrap_turn(text, width):
    """Wrap a turn summary the way Claude Code does, indenting continuation rows."""
    return textwrap.wrap(text, width, subsequent_indent="  ")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--label", default="Session")
    parser.add_argument("--bell-every", type=float,
                        help="Seconds between bell notifications; zero disables them "
                             "(default 10, or 0 with --background)")
    parser.add_argument("--frames", type=int, default=0,
                        help="Stop after this many frames; zero runs until Ctrl+C")
    parser.add_argument("--sparse", action="store_true",
                        help="Put the reply above a blank gap and pin input to the bottom")
    parser.add_argument("--colors", action="store_true",
                        help="Exercise ANSI, indexed/RGB, dim, inverse, and changing preview colors")
    parser.add_argument("--background", action="store_true",
                        help="Cycle a Claude Code footer through working, background tasks, and idle")
    args = parser.parse_args()
    if args.bell_every is None:
        args.bell_every = 0 if args.background else 10
    if args.bell_every < 0 or args.frames < 0:
        parser.error("--bell-every and --frames must be nonnegative")
    if args.colors + args.sparse + args.background > 1:
        parser.error("--colors, --sparse, and --background are separate demo modes")

    # Enable VT processing when run directly in a Windows console.
    old_mode = None
    if os.name == "nt" and sys.stdout.isatty():
        import ctypes
        from ctypes import wintypes

        kernel = ctypes.WinDLL("kernel32", use_last_error=True)
        kernel.GetStdHandle.argtypes = [wintypes.DWORD]
        kernel.GetStdHandle.restype = wintypes.HANDLE
        kernel.GetConsoleMode.argtypes = [wintypes.HANDLE, ctypes.POINTER(wintypes.DWORD)]
        kernel.SetConsoleMode.argtypes = [wintypes.HANDLE, wintypes.DWORD]
        handle = kernel.GetStdHandle(-11 & 0xFFFFFFFF)
        mode = wintypes.DWORD()
        if kernel.GetConsoleMode(handle, ctypes.byref(mode)):
            old_mode = mode.value
            kernel.SetConsoleMode(handle, old_mode | 0x0004)

    started = last_bell = time.monotonic()
    frame = 0
    try:
        sys.stdout.write("\x1b[?1049h\x1b[?25l\x1b[2J")
        while args.frames == 0 or frame < args.frames:
            columns, rows = shutil.get_terminal_size()
            elapsed = time.monotonic() - started
            lines = [
                f"{args.label}: live TUI",
                f"Frame {frame:06d}",
                f"Grid {columns} x {rows}",
                f"Elapsed {elapsed:.1f}s",
                "Output rewrites in place",
                "Long text becomes short",
                "" if frame % 2 else "This row alternates with blank",
                "Working..." if frame % 2 else "Ready for input?",
                "Switch tabs: counter keeps running",
                f"Bottom row: {frame:06d}",
            ]
            if args.sparse:
                reply = f"{args.label}: Hi! What would you like to work on?"
                sys.stdout.write(f"\x1b[2;1H\x1b[2K{reply[:max(0, columns - 1)]}")
                lines = [f"> Ask anything ({frame:06d})", f"Grid {columns} x {rows}"]
            if args.colors:
                lines = [
                    f"{args.label}: terminal colors",
                    "ANSI red",
                    "Bright green",
                    "256-color orange",
                    "RGB coral",
                    "Dim foreground",
                    "Reverse video",
                    "Unicode: \u4e16\U0001f600e\u0301",
                    "Same text; changing color",
                    f"Frame {frame:06d}",
                ]
                colors = ["0", "31", "92", "38;5;208", "38;2;238;146;146",
                          "2", "7", "36", "31" if frame % 4 < 2 else "32", "0"]
                spinner = "\u280b\u2819\u2839\u2838\u283c\u2834\u2826\u2827\u2807\u280f"[frame % 10]
                sys.stdout.write(f"\x1b]2;{spinner} {args.label}\x1b\\")
            if args.background:
                # Each 20 second cycle: working for 2 seconds, waiting on
                # background tasks for 8, then idle for 10. Like Claude Code,
                # turn summaries wrap onto indented rows and the footer is
                # clipped, so a narrow pane exercises both. An earlier turn's
                # "still running" note stays in view throughout.
                second = elapsed % 20
                width = max(10, columns - 1)
                lines = [f"{args.label} ({columns} cols): Claude Code background tasks"]
                lines += wrap_turn("\u273b Crunched for 6s \u00b7 done 5:55 PM \u00b7 1 shell, 1 monitor still running", width)
                lines.append("\u25cf Monitor event: RESULT after ample")
                if second < 2:
                    glyph = "\u00b7\u2722*\u2736\u273b\u273d"[frame % 6]
                    lines.append(f"{glyph} Brewing\u2026 ({int(second) + 1}s \u00b7 \u2193 120 tokens)")
                    hint = "1 shell, 1 monitor \u00b7 esc to interrupt"
                elif second < 10:
                    lines += wrap_turn("\u273b Brewed for 2s \u00b7 done 5:58 PM \u00b7 1 shell, 1 monitor still running", width)
                    hint = "1 shell, 1 monitor \u00b7 \u2193 to manage"
                else:
                    lines += wrap_turn("\u273b Cooked for 3s \u00b7 done 5:59 PM", width)
                    hint = "? for shortcuts"
                lines += ["\u2500" * width, "\u276f", "\u2500" * width, f"  \u23f5\u23f5 auto mode on \u00b7 {hint}"]
                # Keep a fixed height so rewrapped rows never leave stale text.
                lines = [""] * max(0, 14 - len(lines)) + lines
            first_row = max(1, rows - len(lines) + 1)
            for row, text in enumerate(lines[-rows:], start=first_row):
                # Avoid wrapping at the final column, which would scroll the TUI.
                color_index = max(0, len(lines) - rows) + row - first_row
                color = f"\x1b[{colors[color_index]}m" if args.colors else ""
                sys.stdout.write(f"\x1b[{row};1H\x1b[2K{color}{text[:max(0, columns - 1)]}\x1b[0m")
            if args.bell_every and time.monotonic() - last_bell >= args.bell_every:
                sys.stdout.write("\a")
                last_bell = time.monotonic()
            sys.stdout.flush()
            frame += 1
            time.sleep(0.25)
    except KeyboardInterrupt:
        pass
    finally:
        sys.stdout.write("\x1b[?25h\x1b[?1049l")
        sys.stdout.flush()
        if old_mode is not None:
            kernel.SetConsoleMode(handle, old_mode)


if __name__ == "__main__":
    main()
