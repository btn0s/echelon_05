# AGENTS.md — Plugins/ALS-Refactored/Source/

Start with **[`../SYSTEM_MAP.md`](../SYSTEM_MAP.md)** — plugin-wide architecture and flows.

**Packaged assets:** **[`../Content/AGENTS.md`](../Content/AGENTS.md)** — levels, input actions, starter AI assets under **`../Content/`** as binary **`*.uasset` / `*.umap`**. Blueprint / AnimBP / Widget graph summaries can be inspected through UE-MCP package paths under `/ALS`; they are still not text in the repo, and BT/BB semantics may need Editor inspection.

## Module → folder

| UE module | Path |
|-----------|------|
| **ALS** | [`ALS`](ALS/) — `AAlsCharacter`, `UAlsCharacterMovementComponent`, `UAlsAnimationInstance`, notifies, RigVM units (`Nodes/`), settings (`Public/Settings/`), gameplay tags (`Public/Utility/AlsGameplayTags.h`). |
| **ALSCamera** | [`ALSCamera`](ALSCamera/) — `UAlsCameraComponent`, debug overlays. |
| **ALSExtras** | [`ALSExtras`](ALSExtras/) — `AAlsCharacterExample`, `AAlsAIController`. |
| **ALSEditor** | [`ALSEditor`](ALSEditor/) — uncooked animation graph/editor extensions. |

## Naming pitfall

`ECC_Visibility` in **foot offset Rig traces** is a collision channel label, **not** a stealth visibility meter — see `SYSTEM_MAP.md` section 7.
