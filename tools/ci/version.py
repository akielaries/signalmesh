#!/usr/bin/env python3
# minimal reader for a module version.yml - no external deps (ubuntu runners may
# not have PyYAML). prints the newest (top) release's version or its notes.
#
# usage:
#   version.py --version <version.yml>   -> prints e.g. 1.0.0
#   version.py --notes   <version.yml>   -> prints the notes, one "- ..." per line

import sys


def top_entry(path):
    version = None
    notes = []
    in_top = False
    in_notes = False
    with open(path) as f:
        for raw in f:
            stripped = raw.strip()
            if stripped.startswith("- version:"):
                if version is not None:
                    break  # reached the next (older) entry
                version = stripped.split("version:", 1)[1].strip().strip("\"'")
                in_top = True
                in_notes = False
            elif in_top and stripped.startswith("notes:"):
                in_notes = True
            elif in_top and in_notes and stripped.startswith("- "):
                notes.append(stripped[2:].strip())
    return version, notes


def main():
    if len(sys.argv) != 3 or sys.argv[1] not in ("--version", "--notes"):
        sys.exit("usage: version.py --version|--notes <version.yml>")
    version, notes = top_entry(sys.argv[2])
    if version is None:
        sys.exit("no version entry in %s" % sys.argv[2])
    if sys.argv[1] == "--version":
        print(version)
    else:
        print("\n".join("- " + n for n in notes))


if __name__ == "__main__":
    main()
