#!/usr/bin/env bash
# Proves the module-architecture constraint for pygomo_core: the core includes no process, stream,
# thread or protocol-name header. Exit 0 = clean, 1 = violation(s) printed.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
dirs=("$root/include/pygomo/core" "$root/src/core")

# System headers the core must not use: I/O, threads, processes, OS APIs.
forbidden_system='<(iostream|istream|ostream|fstream|sstream|iosfwd|cstdio|stdio\.h|thread|mutex|shared_mutex|condition_variable|future|atomic|stop_token|semaphore|latch|barrier|coroutine|unistd\.h|windows\.h|sys/[^>]*|fcntl\.h|poll\.h|spawn\.h|signal\.h|csignal|filesystem|process)>'
# Protocol or engine names must not appear in any #include line of the core.
forbidden_names='#include[[:space:]]*["<][^">]*(gomocup|piskvork|gtp|rapfi|katago|katagomo|yixin|protocol|transport|adapter)'

status=0
for d in "${dirs[@]}"; do
  [ -d "$d" ] || { echo "missing directory: $d" >&2; exit 2; }
  if grep -rnE "#include[[:space:]]*$forbidden_system" "$d"; then status=1; fi
  if grep -rnEi "$forbidden_names" "$d"; then status=1; fi
done

if [ "$status" -ne 0 ]; then
  echo "check-core-deps: FAIL (pygomo_core must stay protocol- and I/O-free)" >&2
else
  echo "check-core-deps: OK"
fi
exit "$status"
