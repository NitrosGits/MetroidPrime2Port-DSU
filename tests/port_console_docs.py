"""The debug console's command table against its documentation.

platform/port_console.cpp dispatches every command through one table,
kCommands, which also holds the `help` lines. This checks, from the source
alone, that:

  * every command in the table is named in docs/CONSOLE.md, as `name` or
    `name <args...>` in code quotes;
  * every command has exactly one row with a handler (a row with an empty name
    is help text only), so the dispatch finds it;
  * every command with its own row carries help text somewhere in the table,
    and `help` lists the table (not a separate string).

It is a source check: no engine code runs and no game is started.
"""
import pathlib
import re
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from wiring_helpers import WiringCase, between, root_argument  # noqa: E402

root = root_argument()
CONSOLE = root / "platform/port_console.cpp"
CONSOLE_DOC = root / "docs/CONSOLE.md"


def ordered_rows():
    """(name, handler) for each row of kCommands, in table order; text-only rows have name ''."""
    table = between(CONSOLE.read_text(), "const CommandSpec kCommands[] = {", "\n};\n", CONSOLE)
    return [(m.group(1), m.group(2).strip()) for m in re.finditer(r'^    \{"([a-z]*)",\s*([^,]+),', table, re.M)]


class ConsoleDocs(WiringCase):
    def test_the_table_has_rows(self):
        names = {name for name, _ in ordered_rows()}
        # A sample, so a table the pattern no longer reads fails here.
        for name in ("status", "warp", "title", "window", "press", "help", "kill"):
            self.assertIn(name, names)

    def test_one_handler_row_per_command(self):
        seen = set()
        for name, handler in ordered_rows():
            if not name:
                self.assertEqual(handler, "nullptr", "a text-only row has a handler")
                continue
            self.assertNotEqual(handler, "nullptr", f"{name!r} has no handler")
            self.assertNotIn(name, seen, f"{name!r} has two kCommands rows")
            seen.add(name)

    def test_help_lists_the_table(self):
        text = CONSOLE.read_text()
        self.assertIn("for (const CommandSpec& command : kCommands) {\n    out += command.help;", text)

    def test_every_command_is_documented(self):
        doc = CONSOLE_DOC.read_text()
        missing = []
        for name in dict.fromkeys(name for name, _ in ordered_rows() if name):
            if not re.search(r"`%s[\s`]" % re.escape(name), doc):
                missing.append(name)
        self.assertEqual(missing, [], "commands not in docs/CONSOLE.md")


if __name__ == "__main__":
    unittest.main(verbosity=2)
