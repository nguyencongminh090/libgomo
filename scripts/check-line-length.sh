#!/usr/bin/env bash
# Lists every C++ line over 100 columns (characters, UTF-8 aware) in the given directories or files.
# Usage: scripts/check-line-length.sh [dir|file...]   (default: include src tests)
# Exit 0 = clean, 1 = long line(s) printed as file:line:text, 2 = bad argument or grep error.
# Rule: .claude/rules/coding-style.md. third_party/ is never scanned.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"
limit=100

paths=("$@")
[ "${#paths[@]}" -gt 0 ] || paths=(include src tests)

status=0
scan() {
    local rc=0
    LC_ALL=C.UTF-8 grep -nHP "^.{$((limit + 1)),}" "$1" || rc=$?
    case "$rc" in
        0) status=1 ;;
        1) ;;
        *) echo "check-line-length: grep failed (exit $rc) on $1" >&2; exit 2 ;;
    esac
}
for p in "${paths[@]}"; do
    if [ -f "$p" ]; then
        scan "$p"
    elif [ -d "$p" ]; then
        while IFS= read -r -d '' f; do scan "$f"; done < <(
            find "$p" -path '*/third_party' -prune -o -type f \
                 \( -name '*.hpp' -o -name '*.h' -o -name '*.cpp' \) -print0 | sort -z)
    else
        echo "missing path: $p" >&2
        exit 2
    fi
done

if [ "$status" -ne 0 ]; then
    echo "check-line-length: FAIL (lines over $limit columns)" >&2
else
    echo "check-line-length: OK"
fi
exit "$status"
