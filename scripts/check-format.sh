#!/usr/bin/env bash
# Checks that include/, src/ and tests/ are clang-format clean (config: .clang-format, pinned 21.1.8).
# Usage: scripts/check-format.sh   Exit 0 = clean, non-zero = a file would change or clang-format failed.
# Rule: .claude/rules/coding-style.md. third_party/ and tests/fixtures are never checked.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

find include src tests -path '*/fixtures' -prune -o -type f \( -name '*.cpp' -o -name '*.hpp' \) -print0 |
    xargs -0 clang-format --dry-run -Werror
echo "check-format: OK"
