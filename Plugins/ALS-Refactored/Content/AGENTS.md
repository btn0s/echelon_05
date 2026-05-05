# AGENTS.md — Plugins/ALS-Refactored/Content/

Shipped ALS **Fab/GitHub-style** packaged assets under this folder only: **`*.uasset`**, **`*.umap`**. Paths and categories are **`Verified`** from the filesystem; **Blueprint / AnimBP / Widget graph summaries are inspectable through UE-MCP** when the editor bridge is live, but are still not text-readable in git. Behavior Tree / Blackboard semantics and deep asset defaults may still need direct Editor inspection.

## Top-level buckets

| Path | Typical contents |
|------|-------------------|
| `ALS/` | Core locomotion content: animations, overlay poses, mantle/montages, footsteps audio, **`Data/Input`** (`IA_Als_*`). |
| `ALSExtras/` | Playground/grid **levels** (`Levels/*.umap`), environment meshes/audio, **`AI/`** (`AIC_Als`, `BB_Als`), extra input (`Data/Input/`). |
| `ALSCamera/` | Camera-facing data (e.g. blend curves under `Data/`). |

## Blueprint inspection via UE-MCP

Known plugin package mount: filesystem `Plugins/ALS-Refactored/Content/...` maps to Unreal package paths under `/ALS/...`.

Use:

- `blueprint(action="read", assetPath="/ALS/...")` for parent class, components, and tick shape.
- `blueprint(action="list_graphs", assetPath="/ALS/...")` for graph names and node counts.
- `blueprint(action="read_graph_summary", assetPath="/ALS/...", graphName="...")` for node / edge summaries before full graph reads.

Focused inventories:

- [`ALS/Character/AGENTS.md`](ALS/Character/AGENTS.md) — `B_Als_Character`, main/linked/overlay animation Blueprints.
- [`ALSExtras/AGENTS.md`](ALSExtras/AGENTS.md) — sample AI, controller, environment, editor utility, and widget Blueprints.
- [`ALSCamera/AGENTS.md`](ALSCamera/AGENTS.md) — camera component and camera animation Blueprints.

## World Partition payloads

`__ExternalActors__/` and `__ExternalObjects__/` hold map sub-objects for **`ALSExtras/Levels/`** (e.g. `L_Als_Playground`, `L_Als_Grid`).

## Agent rules

1. **Do not narrate node graphs from filenames** — use UE-MCP reads first, then cite **Gap** and point to Editor or [`SYSTEM_MAP.md`](../SYSTEM_MAP.md) section 16 only for details not exposed by the bridge.
2. Prefer cross-referencing **C++ entry points** (e.g. `AAlsCharacterExample` property names) when explaining *how* input *could* bind; UE-MCP can confirm graph nodes and links, while class default values may still need focused property reads.
3. When upgrading ALS from upstream, diff **this tree** against release notes — redirects in `Config/DefaultALS.ini` may affect legacy asset paths.
