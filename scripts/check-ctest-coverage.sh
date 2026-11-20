#!/usr/bin/env bash
# Fails when a doctest case in a ctest-registered test binary is run by no ctest entry.
# Usage: scripts/check-ctest-coverage.sh [build-dir]   (default: build/default, must be built)
# An entry without -tc= covers its whole binary. -tc= values are doctest wildcards (* ?),
# comma separated, matched case-insensitively like doctest does by default.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="${1:-$root/build/default}"
[[ -d "$build" ]] || { echo "check-ctest-coverage: no build dir $build" >&2; exit 2; }

shopt -s nocasematch
declare -A whole            # binary -> 1 when an entry runs it unfiltered
declare -A filters          # binary -> newline separated patterns
while IFS= read -r line; do
  [[ "$line" =~ ^[0-9]+:\ Test\ command:\ (.*)$ ]] || continue
  cmd="${BASH_REMATCH[1]//\"/}"
  read -ra words <<<"$cmd"
  bin="${words[0]}"
  [[ -x "$bin" ]] || continue            # cmake -E, scripts etc. have no doctest cases
  tc=""
  for w in "${words[@]:1}"; do [[ "$w" == -tc=* || "$w" == --test-case=* ]] && tc="${w#*=}"; done
  if [[ -z "$tc" ]]; then whole[$bin]=1; else filters[$bin]+="${tc//,/$'\n'}"$'\n'; fi
done < <(cd "$build" && ctest -N -V)

status=0
bins=$( { printf '%s\n' "${!whole[@]}" "${!filters[@]}"; } | sort -u | sed '/^$/d')
for bin in $bins; do
  [[ -n "${whole[$bin]:-}" ]] && continue
  mapfile -t pats < <(printf '%s' "${filters[$bin]}" | sed '/^$/d')
  while IFS= read -r name; do
    [[ -z "$name" ]] && continue
    hit=0
    for p in "${pats[@]}"; do
      q="${p//\\/\\\\}"; q="${q//\[/\\[}"
      # shellcheck disable=SC2053
      if [[ "$name" == $q ]]; then hit=1; break; fi
    done
    if (( ! hit )); then
      echo "not run by any ctest entry: [$(basename "$bin")] $name"; status=1
    fi
  done < <("$bin" --no-version -ltc | sed -e '/^\[doctest\]/d' -e '/^=\{10,\}/d')
done
(( status == 0 )) && echo "check-ctest-coverage: every doctest case is run by a ctest entry"
exit $status
