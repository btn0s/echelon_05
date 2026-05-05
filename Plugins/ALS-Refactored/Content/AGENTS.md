# AGENTS.md — Plugins/ALS-Refactored/Content/

Shipped ALS **Fab/GitHub-style** packaged assets under this folder only: **`*.uasset`**, **`*.umap`**. Paths and categories are **`Verified`** from the filesystem; **Blueprint / AnimBP / BT / Widget graphs are binary opaque** unless opened in Unreal Editor (not readable as text in git).

## Top-level buckets

| Path | Typical contents |
|------|-------------------|
| `ALS/` | Core locomotion content: animations, overlay poses, mantle/montages, footsteps audio, **`Data/Input`** (`IA_Als_*`). |
| `ALSExtras/` | Playground/grid **levels** (`Levels/*.umap`), environment meshes/audio, **`AI/`** (`AIC_Als`, `BB_Als`), extra input (`Data/Input/`). |
| `ALSCamera/` | Camera-facing data (e.g. blend curves under `Data/`). |

## World Partition payloads

`__ExternalActors__/` and `__ExternalObjects__/` hold map sub-objects for **`ALSExtras/Levels/`** (e.g. `L_Als_Playground`, `L_Als_Grid`).

## Agent rules

1. **Do not narrate node graphs from filenames** — cite **Gap** and point to Editor or [`SYSTEM_MAP.md`](../SYSTEM_MAP.md) section 16.
2. Prefer cross-referencing **C++ entry points** (e.g. `AAlsCharacterExample` property names) when explaining *how* input *could* bind; asset wiring is still manual verification.
3. When upgrading ALS from upstream, diff **this tree** against release notes — redirects in `Config/DefaultALS.ini` may affect legacy asset paths.
