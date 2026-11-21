# WARNING - read before using `latex-document-skill` in another workspace

This skill was copied into the KnowledgeTree from another project. It may contain hardcoded links, absolute paths,
repository names, file conventions or branch rules that belong to its original project.
Remember to redirect them to the workspace you are copying it into.

- Original location: `P/Python/Personal/MachineLearning/.claude/skills/latex-document-skill`
- Identical copies collapsed into this one: 4
- Hardcoded-looking references found: 8 (first ones below)

Detected references:
- `references/tables-and-images.md:166: \usepackage{wrapfig}     % Text wrapping around images`
- `references/tables-and-images.md:256: \usepackage{wrapfig}`
- `references/tables-and-images.md:258: \begin{wrapfigure}{r}{0.3\textwidth}`
- `references/tables-and-images.md:262: \end{wrapfigure}`
- `references/packages.md:123: | `wrapfig` | Wrap text around figures |`
- `references/python-charts.md:1057: \usepackage{wrapfig}`
- `references/python-charts.md:1059: \begin{wrapfigure}{r}{0.5\textwidth}`
- `references/python-charts.md:1064: \end{wrapfigure}`

Checklist before use:
1. Replace absolute paths and project or repo names with the target workspace's.
2. Check that referenced files and folders (TODO.md, CLAUDE.md, docs/, audit or log folders) exist in the target workspace, or adjust the skill.
3. Compare branch, commit and review conventions with the target workspace's rules.
4. Delete this warning file from the copy once the skill has been adapted.

## Known conflicts with knowledge notes

Reviewed 2026-11-21. Where this skill and a note disagree, follow the note and tell the user why. The skill files themselves are kept verbatim.

- `fmt-pdf-accessibility-pdfa`: `--pdfa` produces PDF/A-2b, and the skill's trigger list says "make a document accessible (PDF/A, tagged PDF)". PDF/A level b only guarantees visual reproduction; it is not accessible and its text may not be searchable. Prefer PDF/A-2u, or 2a when the document is tagged, add PDF/UA only where required, and validate the output (veraPDF). The skill's own `references/accessibility-guide.md` already distinguishes 2b from 2a.
- `tex-tables-and-figures`: `references/tables-and-images.md` shows `\begin{tabular}{|l|c|r|}` with `\hline` between every row, and the same for `tabularx` and `\rowcolor` examples. The booktabs manual says never to use vertical rules or double rules and to use `\toprule`, `\midrule`, `\bottomrule`; use booktabs for new tables (the skill lists the package but does not use it in the basic example).
- `tex-tables-and-figures`: SKILL.md advises `\begin{figure}[H]` (package `float`) "to prevent figures from floating". Prefer `[tbp]` placement; `[H]` leaves gaps and, without `\usepackage{float}`, fails with `Unknown float option 'H'`. Use it only for a figure that must stay between two paragraphs.

## Adapt before use

- Vietnamese: the templates and `references/font-guide.md` use `\usepackage[utf8]{inputenc}` with `\usepackage[T1]{fontenc}`. T1 has no Vietnamese glyphs (compile error `Unicode character ... not set up for use with LaTeX`). Use `T5` with `[vietnamese]{babel}` on pdfLaTeX, or XeLaTeX with `fontspec`: see `tex-vietnamese-and-unicode`.

