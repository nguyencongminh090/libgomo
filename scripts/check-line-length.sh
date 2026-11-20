#!/usr/bin/env bash
# Lists every C++ line over 100 columns (characters, UTF-8 aware) in the given directories.
# Usage: scripts/check-line-length.sh [dir...]   (default: include src tests)
# Exit 0 = clean, 1 = long line(s) printed as file:line:text, 2 = bad argument.
# Rule: .claude/rules/coding-style.md. third_party/ is never scanned.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"
limit=100

dirs=("$@")
[ "${#dirs[@]}" -gt 0 ] || dirs=(include src tests)

status=0
for d in "${dirs[@]}"; do
    [ -d "$d" ] || { echo "missing directory: $d" >&2; exit 2; }
    while IFS= read -r -d '' f; do
        if LC_ALL=C.UTF-8 grep -nHP "^.{$((limit + 1)),}" "$f"; then status=1; fi
    done < <(find "$d" -path '*/third_party' -prune -o -type f \
                 \( -name '*.hpp' -o -name '*.h' -o -name '*.cpp' \) -print0 | sort -z)
done

if [ "$status" -ne 0 ]; then
    echo "check-line-length: FAIL (lines over $limit columns)" >&2
else
    echo "check-line-length: OK"
fi
exit "$status"
