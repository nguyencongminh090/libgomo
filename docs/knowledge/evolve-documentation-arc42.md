---
name: evolve-documentation-arc42
description: Documenting architecture with the arc42 template - its twelve sections, the lean / essential / thorough usage levels, and how decisions, quality scenarios and risks fit in
domain: software-architecture
tags: evolve,documentation,arc42,template,communication
apply_when: "a project needs a structure for architecture documentation, or existing documents are scattered and no one knows what is missing"
sources: "arc42 documentation, docs.arc42.org: home, section 9 Architecture Decisions, section 11 Risks and Technical Debt (read; CC BY-SA 4.0; created by Gernot Starke, supported by Kroner & Starke and INNOQ)"
last_reviewed: 2026-09-21
confidence: medium
---

# Documenting architecture with arc42

## What it is

arc42 is a free, open-source template for architecture communication and documentation,
licensed CC BY-SA 4.0 and maintained on GitHub. It is a structure of twelve sections to fill
in, not a notation: use it with diagrams from C4 or UML (`foundation-c4-model`,
`foundation-views-4plus1`).

## The twelve sections

| # | Section | Holds |
|---|---|---|
| 1 | Introduction and goals | requirements overview, stakeholders' needs, top quality goals |
| 2 | Constraints | technical and organizational limits, conventions |
| 3 | Context and scope | business and technical environment, external interfaces |
| 4 | Solution strategy | core decisions and approaches |
| 5 | Building block view | static decomposition of the code into abstractions |
| 6 | Runtime view | how building blocks interact at runtime |
| 7 | Deployment view | infrastructure, hardware, mapping of software to it |
| 8 | Crosscutting concepts | recurring patterns and solution approaches used across the system |
| 9 | Architecture decisions | significant, expensive, risky or disputed decisions |
| 10 | Quality requirements | overview and quality scenarios |
| 11 | Risks and technical debt | prioritized known problems, risks and debts |
| 12 | Glossary | business and technical terms |

Rough correspondence with other notes: 1 and 10 with `foundation-significant-requirements` and
`quality-scenarios-utility-tree`; 3 and 5 with C4 context, container and component levels;
7 with Kruchten's physical view; 9 with `evolve-adr`; 11 with `evolve-technical-debt`.

## Usage levels

arc42 offers three pragmatic approaches: **lean** for agile, lightweight documentation,
**thorough** for formal or critical systems, and **essential** for the fundamental information
every system should include.

## Section 9 and 11 guidance

- Section 9: record architecturally significant decisions (structure, quality characteristics,
  dependencies, interfaces, construction techniques), especially where the motivation is not
  visible in code; keep it lean and avoid duplicating the building block sections; formats:
  ADRs, a table by importance, or sections per decision.
- Section 11: a prioritized list or table of risks and technical debts, with mitigations;
  identify them from stakeholder views, external interfaces, processes, data structures and
  source code.

## Use when

- Starting documentation for a new system, or restructuring scattered documents.
- You need a completeness checklist: which questions does our documentation answer?
- Reviewing an inherited system: filling the sections shows what is unknown.

## Do not use when

- A prototype or very small tool: a short README and a few diagrams suffice.
- You expect the template to produce good architecture; it only organizes what you write.
- A standards-compliant description with formal viewpoints and stakeholder-concern mapping is
  required: use it alongside `foundation-42010-concepts`, not as proof of conformance.
- The team will not maintain it: keep to the lean level and store it with the code.

## Trade-offs

- A common structure helps readers find things and shows gaps, but filling all twelve sections
  for a small system is wasted effort.
- Documents next to the code stay closer to reality; wikis tend to drift.

## Common mistakes

- Diagrams with no explanation of the reasons behind the structure.
- Never updating sections 9 and 11, so they mislead.

## Verification note

Structure, levels and section guidance were read on the template's own site. No independent
source describing the template was read, and the arc42 site content for the remaining sections
was not opened in detail. `confidence: medium`.

## Related

- Notes: `evolve-adr`, `evolve-technical-debt`, `foundation-c4-model`, `foundation-views-4plus1`,
  `foundation-42010-concepts`, `foundation-significant-requirements`
