#!/usr/bin/env bash
# Proves two module-architecture constraints. Exit 0 = clean, 1 = violation(s) printed.
#
#  1. gomo_core includes no process, stream, thread or protocol-name header.
#  2. The transport port headers (include/gomo/transport/) mention no protocol or engine name:
#     the port must stay protocol-blind.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
core_dirs=("$root/include/gomo/core" "$root/src/core")
transport_dir="$root/include/gomo/transport"

# System headers the core must not use: I/O, threads, processes, OS APIs.
forbidden_system='<(iostream|istream|ostream|fstream|sstream|iosfwd|cstdio|stdio\.h|thread|mutex|shared_mutex|condition_variable|future|atomic|stop_token|semaphore|latch|barrier|coroutine|unistd\.h|windows\.h|sys/[^>]*|fcntl\.h|poll\.h|spawn\.h|signal\.h|csignal|filesystem|process)>'
# Protocol or engine names must not appear in any #include line of the core.
forbidden_include_names='#include[[:space:]]*["<][^">]*(gomocup|piskvork|gtp|rapfi|katago|katagomo|yixin|protocol|transport|adapter)'
# Protocol or engine names must not appear anywhere in the transport port headers.
forbidden_port_names='gomocup|piskvork|gtp|rapfi|katago|yixin'

status=0
for d in "${core_dirs[@]}"; do
  [ -d "$d" ] || { echo "missing directory: $d" >&2; exit 2; }
  if grep -rnE "#include[[:space:]]*$forbidden_system" "$d"; then status=1; fi
  if grep -rnEi "$forbidden_include_names" "$d"; then status=1; fi
done

[ -d "$transport_dir" ] || { echo "missing directory: $transport_dir" >&2; exit 2; }
if grep -rnEiw "$forbidden_port_names" "$transport_dir"; then status=1; fi

if [ "$status" -ne 0 ]; then
  echo "check-core-deps: FAIL (core must stay protocol- and I/O-free; transport port must stay protocol-blind)" >&2
else
  echo "check-core-deps: OK"
fi
exit "$status"
