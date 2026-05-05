# AGENTS.md — echelon_05 root

Minimal project pointer. **Linear TH-505** (locomotion / ALS navigation) is scoped to **plugin docs and paths under** [`Plugins/ALS-Refactored`](Plugins/ALS-Refactored/) only — not host game `Source/`, root `Config/`, or **repo-root** `Content/` (those `.uasset`/`.umap` trees are separate from the plugin).

For ALS behavior, configs, and file-level navigation, agents should work **only under** [`Plugins/ALS-Refactored`](Plugins/ALS-Refactored/) unless the task explicitly expands scope.

## ALS documentation (canonical for TH-505 / locomotion)

- **[`Plugins/ALS-Refactored/SYSTEM_MAP.md`](Plugins/ALS-Refactored/SYSTEM_MAP.md)** — full plugin system map (evidence legend, subsystems, Mermaid diagram, gaps).
- **[`Plugins/ALS-Refactored/AGENTS.md`](Plugins/ALS-Refactored/AGENTS.md)** — module map and upgrade/read-first notes for the plugin subtree.
- **[`Plugins/ALS-Refactored/Content/AGENTS.md`](Plugins/ALS-Refactored/Content/AGENTS.md)** — packaged `*.uasset` / `*.umap` layout under the plugin only (binary assets; graphs need Editor).

## Optional Cursor canvas

IDE-local companion (not necessarily git-tracked): `.cursor/projects/.../canvases/echelon-als-system-map.canvas.tsx` — aligned with plugin `SYSTEM_MAP.md`.
