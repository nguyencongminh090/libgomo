# WARNING - read before using `adr-author` in another workspace

This skill was copied into the KnowledgeTree from a public repository. It may contain hardcoded links, absolute paths,
repository names, file conventions or branch rules that belong to its original project.
Remember to redirect them to the workspace you are copying it into.

- Original location: https://github.com/microsoft/hve-core/tree/HEAD/.github/skills/project-planning/adr-author (fetched 2026-11-22, verbatim, commit e13fbf9)
- License: MIT
- Identical copies collapsed into this one: 0
- Hardcoded-looking references found: 4

Product assumptions and requirements:
- Written for the HVE Core "ADR Creator" agent. It reads and writes state under `.copilot-tracking/adr-plans/{projectSlug}/state.json` and `.copilot-tracking/architecture-diagrams/state.json`; these paths are specific to that framework. Direct use runs the bootstrap protocol in `references/adr-identity.md`.
- Bundled Python scripts (`scripts/*.py`, uv/pyproject, tests, fuzz harness) are copied unmodified and were NOT run during import. Review before executing; `scan_sensitive_content.py` and `update_lineage.py` write files. Needs Python 3 with the deps in `pyproject.toml`.
- Overlaps `documentation-and-adrs` (lighter). Use this one for MADR v4 or Y-statement output, supersession lineage and ASR (architecturally significant requirement) triggers.

Checklist before use:
1. Replace absolute paths and project or repo names with the target workspace's.
2. Check that referenced files and folders exist in the target workspace, or adjust the skill.
3. Compare branch, commit and review conventions with the target workspace's rules.
4. Delete this warning file from the copy once the skill has been adapted.
