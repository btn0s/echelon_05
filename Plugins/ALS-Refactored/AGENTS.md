# AGENTS.md — Plugins/ALS-Refactored/

**Linear TH-505:** treat locomotion investigation and navigation as **this plugin subtree only** — packaged locomotion lives under **`Content/`** as binary **`*.uasset` / `*.umap`** (layout and filenames in git; Blueprint / AnimBP graph summaries can be read through UE-MCP when the editor bridge is live).

**Read first:** **[`SYSTEM_MAP.md`](SYSTEM_MAP.md)** — full plugin-scope system map (subsystems, data flow, evidence tags, gaps, Mermaid).

## Fast orientation

| What | Where |
|------|--------|
| Plugin descriptor | [`ALS.uplugin`](ALS.uplugin) |
| Release / feature notes | [`README.md`](README.md) |
| Replication & debug config | [`Config/Engine.ini`](Config/Engine.ini), [`Config/Input.ini`](Config/Input.ini), [`Config/DefaultALS.ini`](Config/DefaultALS.ini) |
| Source map | [`Source/AGENTS.md`](Source/AGENTS.md) |
| Packaged Unreal assets | [`Content/AGENTS.md`](Content/AGENTS.md) — **`*.uasset` / `*.umap`** only under **`Plugins/ALS-Refactored/Content/`** (binary; folder/layout in git, UE-MCP can inspect known Blueprint package paths under `/ALS`) |

## Agent rules of thumb

1. **Do not invent AnimBP / BT / uasset behavior** — use UE-MCP `blueprint(...)` reads for known `/ALS/...` package paths, and cite `SYSTEM_MAP.md` gap register when graph details, CDO defaults, BT/BB semantics, or asset internals are still not exposed.
2. **Preserve upgrade path** — prefer upstream alignment; local C++ edits fork maintenance.
3. **Stay inside this folder** when the task is “ALS only”; do not treat host `Source/echelon_05`, root `Config/`, or **repo-root** `Content/` as part of ALS without explicit ask.

## Upstream

<https://github.com/Sixze/ALS-Refactored> — issues, discussions, release changelogs.
