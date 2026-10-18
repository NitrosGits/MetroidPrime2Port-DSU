#!/usr/bin/env python3
"""Send real key presses to an X display (XTEST), for testing the port's keyboard bindings.

The port reads keys through SDL, so a virtual pad (`MP2_INPUT`, the console's `press`) cannot
exercise them; this puts a real key-down/key-up on the display the game is running on.

    tools/keytap.py :86 p            # tap P
    tools/keytap.py :86 F1 --hold 50 # F1 down for 50 ms, then up
    tools/keytap.py :86 "Left Shift" Return

The display's Xauthority is not used: a headless Xvfb started with `-ac` needs none, and the
session's XAUTHORITY points at a socket a bare Xvfb does not read.
"""
import os
import sys
import time

os.environ.pop("XAUTHORITY", None)

from Xlib import X, XK, display  # noqa: E402
from Xlib.ext import xtest  # noqa: E402


def keycode(d, name):
    keysym = XK.string_to_keysym(name)
    if keysym == 0:
        raise SystemExit(f"keytap: no such keysym: {name}")
    code = d.keysym_to_keycode(keysym)
    if code == 0:
        raise SystemExit(f"keytap: keysym {name} has no keycode on this display")
    return code


def main(args):
    if not args:
        raise SystemExit(__doc__)
    display_name, rest = args[0], args[1:]
    hold = 20
    names = []
    i = 0
    while i < len(rest):
        if rest[i] == "--hold":
            if i + 1 >= len(rest):
                raise SystemExit("keytap: --hold needs a number of milliseconds")
            hold = int(rest[i + 1])
            i += 2
            continue
        names.append(rest[i])
        i += 1
    if not names:
        raise SystemExit(__doc__)
    d = display.Display(display_name)
    if not d.query_extension("XTEST"):
        raise SystemExit("keytap: the display has no XTEST extension")
    for name in names:
        code = keycode(d, name)
        xtest.fake_input(d, X.KeyPress, code)
        d.sync()
        time.sleep(hold / 1000.0)
        xtest.fake_input(d, X.KeyRelease, code)
        d.sync()
        print(f"keytap: {name} (keycode {code})")
        time.sleep(0.1)


if __name__ == "__main__":
    main(sys.argv[1:])
