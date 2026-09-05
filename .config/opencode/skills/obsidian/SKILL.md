---
name: second-brain
description: Update the second-brain/ Obsidian vault (atomic linked notes — decisions, gotchas, subsystems, roadmap, glossary) after ANY change made to this project — code, config, deploy, or docs. Always run this at the end of a task, not just when explicitly asked to "update second-brain", "record this decision", "note this gotcha", or "add this to the vault" — not for editing docs/ (the formal manual) or dashboard/app.py itself.
---

# second-brain vault maintenance

**Run this after every change to the project, unconditionally** — not only
when the user explicitly asks. This is a standing project convention (see
`CLAUDE.md`), the same way you'd run a linter or update a changelog. If a
task's changes don't fit any bucket below (see "Decide what kind of change
this is"), that's a signal to double check, not a reason to skip the vault.

`second-brain/` is an Obsidian vault of short, atomic, wikilinked notes for this
project. It mirrors `docs/` but is **not** a duplicate of it — `docs/` is the
manual, this vault is the mental model. Read `second-brain/Home.md` first; it's
the map of content (MOC) and every note links from it.

Keep entries short (most notes are 20–45 lines) and let notes reference each
other with `[[Wikilinks]]` instead of restating things.

## Decide what kind of change this is

| The change... | Do this |
|---|---|
| Fixes a bug / reveals a sharp edge worth remembering | Add a bullet to `Gotchas.md` → `## Other landmines` (or a new `##` section if it needs more than a line). Don't create a new file for one gotcha. |
| Is a locked architectural/technical choice ("don't do X, we tried it, here's why") | New note `Decision - <Title>.md` (see structure below), then link it from `Home.md` under `## 🧭 Decisions`. |
| Adds or meaningfully changes a subsystem (new feature area, new component) | New note `<Subsystem Name>.md` (Title Case, no prefix), linked from `Home.md` under `## 🧩 Subsystems`. If it's a small addition to an existing subsystem, edit that note instead of creating a new one. |
| Ships or newly plans a roadmap item | Edit `Roadmap.md` — move/add the bullet under `## Recently shipped` or `## Planned / agreed`. |
| Introduces new project vocabulary (a term someone would need defined) | Append a bullet to `Glossary.md`, alphabetical-ish, `[[wikilink]]` to the fuller note if one exists. |
| Changes the request flow, component list, or trust boundary | Edit `Architecture.md`'s flow diagram / component table directly. |

If none of these fit, or the change is genuinely new ground, ask rather than
guessing which note it belongs in.

## Note structures

**Decision notes** (`Decision - <Title>.md`):
```markdown
---
tags: [decision]
---

# Decision — <Title>

**Status:** locked. / **Status:** working agreement (user-directed). / etc.

## Context
Why this came up — the problem, ideally with a concrete incident.

## Decision
What was decided, in one or two sentences. Include the exact config
knob/flag if there is one.

## Consequences
What this costs or requires going forward.

Related: [[Other Note]], [[Another Note]].
```
Add `workflow` or a second topical tag (`[tags: [decision, ops]]`) when it
fits — see the tag list below.

**Subsystem / reference notes**: short intro paragraph, then `##` sections
and/or a table, cross-linked throughout via `[[Note Name]]` or
`[[Note Name|display text]]`. A trailing `Related:` line is common but not
required.

## The Home.md MOC — always update it

Every new note **must** be added to `Home.md` under the right section
(`Start here`, `Subsystems`, or `Decisions`). An orphaned note nobody links to
defeats the vault. If the note introduces a genuinely new tag, add it to the
`## 🏷️ Tags` line too.

## Tags in use

Reuse these instead of inventing new ones unless nothing fits:
`moc`, `home`, `auth`, `rbac`, `sessions`, `recording`, `monitoring`,
`architecture`, `ops`, `decision`, `workflow`, `gotcha`, `reference`,
`roadmap`, `engineering`, `tech-debt`. Notes commonly carry two tags
(`[architecture, auth]`, `[gotcha, ops]`).

## What NOT to do

- Don't copy full prose from `docs/` in — link to it instead:
  `` [`docs/ARCHITECTURE.md`](../docs/ARCHITECTURE.md) ``.
- Don't write a note longer than it needs to be — if it's creeping past
  ~50 lines, it's probably two notes or belongs as a `Related:` link off an
  existing one.
- Don't touch `.obsidian/` (vault config, not content).
- Don't edit `second-brain/` instead of `docs/` when the user actually asked
  for the formal manual updated — they're separate; this skill is for the
  vault only.
