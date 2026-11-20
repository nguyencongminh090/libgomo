#!/usr/bin/env bash
# Lists public declarations in headers that have no `///` doc comment directly above them.
# Usage: scripts/check-doc-comments.sh [dir...]   (default: include)
# Exit 0 = clean, 1 = missing doc(s) printed as file:line:text, 2 = bad argument.
#
# Heuristic, not a parser (rule: .claude/rules/coding-style.md section 2). Checked: struct, class,
# union and enum definitions, `using X =` aliases, free functions and public methods, at namespace
# scope or in a public section. Not checked: data members, enumerators, destructors, `= default` /
# `= delete` members, forward declarations, continuation lines of a multi-line signature.
# `[[attribute]]` and `template <...>` lines between the doc and the declaration are skipped.
# A trailing `///<` on the declaration line also counts.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

dirs=("$@")
[ "${#dirs[@]}" -gt 0 ] || dirs=(include)

files=()
for d in "${dirs[@]}"; do
    [ -d "$d" ] || { echo "missing directory: $d" >&2; exit 2; }
    while IFS= read -r -d '' f; do files+=("$f"); done \
        < <(find "$d" -path '*/third_party' -prune -o -type f \
                \( -name '*.hpp' -o -name '*.h' \) -print0 | sort -z)
done
[ "${#files[@]}" -gt 0 ] || { echo "check-doc-comments: OK (no headers)"; exit 0; }

status=0
awk '
function trim(s) { sub(/^[ \t]+/, "", s); sub(/[ \t]+$/, "", s); return s }
function isPrefix(s) { return s ~ /^\[\[[^]]*\]\]$/ || s ~ /^template[ \t]*<.*>$/ }
function checkable() {
    return sp == 0 || kind[sp] == "ns" || (kind[sp] == "rec" && pub[sp])
}
function isDecl(s) {
    if (s == "" || s ~ /^(\/\/|#|\}|\{)/) return 0
    if (s ~ /^(public|private|protected)[ \t]*:/) return 0
    if (isPrefix(s)) return 0
    if (s ~ /=[ \t]*(default|delete)[ \t]*;/) return 0
    if (s ~ /(^|[ \t])~[A-Za-z_][A-Za-z0-9_]*[ \t]*\(/) return 0
    if (s ~ /^(static_assert|friend|return|namespace|using[ \t]+namespace)([^A-Za-z0-9_]|$)/) return 0
    if (s ~ /^(struct|class|union|enum([ \t]+class)?)[ \t]+[A-Za-z_][A-Za-z0-9_:]*[ \t]*;$/) return 0
    if (s ~ /^(template[ \t]*<.*>[ \t]*)?(struct|class|union|enum)[ \t]/) return 1
    if (s ~ /^using[ \t]+[A-Za-z_][A-Za-z0-9_]*[ \t]*=/) return 1
    if (s ~ /[A-Za-z_0-9>][ \t]*\(/ || s ~ /operator.*\(/) return 1
    return 0
}
function hasDoc(n,    i) {
    if (h[n] ~ /\/\/\/</) return 1
    for (i = n - 1; i >= 1 && isPrefix(h[i]); i--) {}
    return i >= 1 && h[i] ~ /^\/\/\//
}
function kindFor(s) {
    if (s ~ /^(inline[ \t]+)?namespace([^A-Za-z0-9_]|$)/) return "ns"
    if (s ~ /^enum([^A-Za-z0-9_]|$)/) return "enum"
    if (s ~ /^(template[ \t]*<.*>[ \t]*)?(struct|class|union)[ \t]/) return "rec"
    return "other"
}
FNR == 1 { sp = 0; pdepth = 0; delete h }
{
    t = trim($0)
    h[FNR] = t
    if (pdepth == 0 && checkable() && isDecl(t) && !hasDoc(FNR)) {
        print FILENAME ":" FNR ": " t
        bad = 1
    }
    if (sp > 0 && kind[sp] == "rec" && t ~ /^(public|private|protected)[ \t]*:/) {
        pub[sp] = (t ~ /^public/) && outerOk[sp]
    }
    # Count braces and parens outside comments and string or char literals.
    code = $0
    sub(/\/\/.*/, "", code)
    gsub(/"([^"\\]|\\.)*"/, "\"\"", code)
    gsub(/\047([^\047\\]|\\.)*\047/, "\047\047", code)
    first = 1
    for (i = 1; i <= length(code); i++) {
        c = substr(code, i, 1)
        if (c == "(") pdepth++
        else if (c == ")") pdepth--
        else if (c == "{") {
            k = first ? kindFor(t) : "other"
            first = 0
            # A record nested in a private section stays private, whatever its own access.
            outer = checkable()
            sp++
            kind[sp] = k
            outerOk[sp] = outer
            pub[sp] = (k == "rec" && outer && t !~ /^(template[ \t]*<.*>[ \t]*)?class[ \t]/)
        } else if (c == "}" && sp > 0) sp--
    }
}
END { exit bad ? 1 : 0 }
' "${files[@]}" || status=$?

if [ "$status" -eq 1 ]; then
    echo "check-doc-comments: FAIL (public declarations without a /// doc comment)" >&2
elif [ "$status" -ne 0 ]; then
    echo "check-doc-comments: error (awk exit $status)" >&2
else
    echo "check-doc-comments: OK"
fi
exit "$status"
