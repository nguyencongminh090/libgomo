# Coding style: column alignment and comments

Adapted for PyGomo (C++23) from SKILLS_TREE `rules/coding-standards/items/coding-style.md`. Naming and code structure: match
the surrounding code (types `PascalCase`, functions `camelCase`, 4-space indent). Line limit: 100 columns.
Never restyle a file you are only editing; the project-wide pass is its own task.

## 1. Column alignment: a separate format pass, not part of writing

While writing or editing:
- Single space around `=` and before a trailing comment. No padding to line things up.
- Do not touch the alignment of code you did not change. Later edits inside an aligned block are written unaligned.

The align pass runs only when the user types `/align` (or names a task that is an align pass). Never start it on your own;
when work is finished and verified, you may remind the user once that `/align` exists.

1. Scope: the blocks containing lines changed in this task, or the files the user names.
2. Align, inside a contiguous block of 2+ related lines:
   - `=` of consecutive assignments: constants, config, and variable declarations or assignments
     (`auto x = ...;`, `x = ...;`); compound operators (`+=`, `|=`) align on their `=`
   - member types, member names and default initialisers in structs, classes and enums; consecutive declarations
   - trailing comments (`///<` and `//`)
   - arguments of a multi-line call or signature when each line is one record
3. Do not align:
   - across a blank line or an unrelated line (that starts a new block)
   - when one outlier pushes the column more than ~20 characters right: break the block or wrap the outlier
   - past 100 columns
   - prose or single statements
4. Whitespace only: no renames, reordering or logic edits. `third_party/` is never touched.
5. Rebuild and run `ctest --preset default` to confirm nothing else changed.
6. Commit it alone (for example `style: align columns`) so whitespace noise stays out of logic diffs.

Example:

```cpp
struct SearchOutcome {
    std::vector<InfoEvent>   info{};      ///< Progress reports, in arrival order.
    std::vector<SearchEvent> other{};     ///< Message and unparsed lines seen during the search.
    SearchEvent              terminal{};  ///< BestMove, Swap, Pass or Error.
};

auto       deadline = clock.now() + limits.turnTime;
const auto size     = board.size();
moveCount          += 1;
```

The project has no `.clang-format` yet. Adding one (with `AlignConsecutiveAssignments`, `AlignConsecutiveDeclarations`,
`AlignTrailingComments`) is a toolchain change: it needs a planning first.

## 2. Comments

Principle: comment the why, not the what. Brief; one line if it fits. The reader is a contributor who needs the core idea
of a type or function without reading its body.

- Every public type, function and method in `include/` gets a `///` doc comment directly above the declaration.
  Struct and enum members get a trailing `///<` (or a `///` line above when it does not fit).
- First sentence: a summary in the third person ("Runs one search and waits for its terminal event."), ending with a period.
- Then only what the signature does not say: units, ranges, side effects, error cases (which `EngineError`), ownership and
  lifetime, thread-safety, blocking behaviour.
- Do not restate the code (`++i; // increment i`) or the types already in the signature.
- A file whose role is not obvious from its name starts with a short `//` comment saying what it is for and which layer it
  belongs to (core, adapter, transport).
- Private helpers and code in `src/`: a one-line `//` comment, or none if the name says it all.
- Non-obvious code (workaround, invariant, protocol quirk, ordering or locking constraint): a short `//` comment above it,
  with the protocol spec section, ADR or issue id when there is one.
- No commented-out code; git remembers.
- TODOs: `TODO(owner): action`, or `TODO(<todo code>): action` with a harness todo code. No bare TODO.
- Stay inside 100 columns. Update or delete a comment when the code beside it changes.
- Tests: the `TEST_CASE` name says the behaviour; add a comment only for a non-obvious setup or expectation.
