# Module architecture profile

- Moving a module boundary or adding a cross-layer dependency is size L: planning, then an ADR for the decision.
- Write the design doc (`docs/design/`) before moving code; a feature in `docs/features.json` names the dependencies that must not exist afterwards and the command that proves it.
- Prefer many small todos (each at most M) that keep the build green after each one.
