#!/usr/bin/env python3
import sys
from pathlib import Path


def write_c_string_literal(input_path, output_path):
    data = Path(input_path).read_text()
    with Path(output_path).open("w") as out:
        out.write("/* Generated from %s. Do not edit. */\n" % Path(input_path).name)
        for line in data.splitlines(True):
            escaped = line.replace("\\", "\\\\").replace("\"", "\\\"").replace("\n", "\\n")
            out.write('"%s"\n' % escaped)


def main(argv):
    if len(argv) != 3:
        print("usage: embed_text.py INPUT OUTPUT", file=sys.stderr)
        return 2
    write_c_string_literal(argv[1], argv[2])
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
