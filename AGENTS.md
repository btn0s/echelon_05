# AGENTS.md — echelon_05 root

Minimal project pointer. **Linear TH-505** (locomotion / ALS navigation) is scoped to **plugin docs and paths under** [`Plugins/ALS-Refactored`](Plugins/ALS-Refactored/) only — not host game `Source/`, root `Config/`, or **repo-root** `Content/` (those `.uasset`/`.umap` trees are separate from the plugin).

For ALS behavior, configs, and file-level navigation, agents should work **only under** [`Plugins/ALS-Refactored`](Plugins/ALS-Refactored/) unless the task explicitly expands scope.

## ALS documentation (canonical for TH-505 / locomotion)

- **[`Plugins/ALS-Refactored/SYSTEM_MAP.md`](Plugins/ALS-Refactored/SYSTEM_MAP.md)** — full plugin system map (evidence legend, subsystems, Mermaid diagram, gaps).
- **[`Plugins/ALS-Refactored/AGENTS.md`](Plugins/ALS-Refactored/AGENTS.md)** — module map and upgrade/read-first notes for the plugin subtree.
- **[`Plugins/ALS-Refactored/Content/AGENTS.md`](Plugins/ALS-Refactored/Content/AGENTS.md)** — packaged `*.uasset` / `*.umap` layout under the plugin only (binary assets; Blueprint / AnimBP graph summaries can be inspected through UE-MCP when the editor bridge is live).

## Optional Cursor canvas

IDE-local companion (not necessarily git-tracked): `.cursor/projects/.../canvases/echelon-als-system-map.canvas.tsx` — aligned with plugin `SYSTEM_MAP.md`.

## Learned User Preferences

- For TH-505 / ALS locomotion documentation, keep scope limited to `Plugins/ALS-Refactored` unless the user explicitly expands it.

## Learned Workspace Facts

- Git LFS is configured for Unreal assets via `.gitattributes`, including `*.uasset`, `*.umap`, common mesh/image/audio source files, and related binary asset formats.
- `.gitignore` is set up for Unreal-generated folders such as `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`, plugin build folders, and `.cursor/`.
- `Content/__ExternalActors__/...` files are World Partition / One File Per Actor payloads and should be committed with their matching intentional `.umap` instead of globally ignored.
- `package.json` and `scripts/` provide Node helpers for build, launch, clean, log cleanup, rebuild, and dev flows using the UE 5.7 engine path; build, launch, and clean scripts include WSL-aware PowerShell bridging.
- UE-MCP sees the repo-root playground map as `/Game/L_Als_Playground`, while ALS plugin content is mounted under `/ALS` with package paths like `/ALS/ALSExtras/Levels/L_Als_Playground`.
- For ALS plugin Blueprints, use UE package paths under `/ALS` (for example `/ALS/ALS/Character/B_Als_Character`); native `asset(action="list")` may not enumerate plugin mounts, but `blueprint(...)` reads known package paths.
