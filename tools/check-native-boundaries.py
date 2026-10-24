#!/usr/bin/env python3
"""Fail when handwritten native files exceed the agreed responsibility budgets."""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1] / "native"
FOLDERS = ("src", "include", "scenes", "examples", "tests", "assets/shaders")

def limit(path):
    if path.suffix in (".vert", ".frag"):
        return 180
    if path.suffix == ".hpp":
        return 260
    if path.suffix != ".cpp":
        return None
    if "tests" in path.relative_to(ROOT).parts:
        return 420
    return 320 if "qt" in path.relative_to(ROOT).parts else 360

def main():
    failures = []
    checked = 0
    for folder in FOLDERS:
        for path in (ROOT / folder).rglob("*"):
            maximum = limit(path)
            if maximum is None or not path.is_file():
                continue
            checked += 1
            count = len(path.read_text().splitlines())
            if count > maximum:
                failures.append(f"{path.relative_to(ROOT)}: {count} > {maximum}")
            if folder == "include" and "/lighting/" in path.as_posix():
                if "#include <glad" in path.read_text() or "#include <Q" in path.read_text():
                    failures.append(f"{path}: CPU lighting may not depend on OpenGL or Qt")
    if failures:
        print("\n".join(failures), file=sys.stderr)
        return 1
    print(f"PASS native boundaries: {checked} handwritten files within line limits")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
