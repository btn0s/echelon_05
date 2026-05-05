# AGENTS.md — Plugins/ALS-Refactored/Content/ALS/

Core ALS locomotion content. Files are binary `*.uasset`; use UE-MCP package paths under `/ALS/ALS/...` for asset inspection.

## Key Buckets

| Path | Typical contents |
|------|------------------|
| `Character/` | Character Blueprint, main and linked AnimBPs, overlay AnimBPs. See [`Character/AGENTS.md`](Character/AGENTS.md). |
| `Animations/` | Animation sequences, montages, blendspaces, transition clips, overlay poses. |
| `Data/` | Character, animation-instance, input, footstep, movement, and mantle tuning assets. |
| `OverlayObjects/` | Skeletal/static overlay meshes and overlay-object-specific assets, including bow animation Blueprint. |
| `Audio/Footsteps/` | Footstep MetaSound / audio assets. |
| `Editor/` | Editor utility assets such as foot sync marker modifier. |

## UE-MCP Notes

- `B_Als_Character` at `/ALS/ALS/Character/B_Als_Character` is the main Blueprint sample character and is readable with `blueprint(...)`.
- `AB_Als` and linked animation assets under `/ALS/ALS/Character/AnimationInstances/...` expose graph lists and many graph summaries through UE-MCP.
- `OverlayObjects/Bow/AB_Als_Bow` is also an AnimBP and was readable at graph-list level.

## Behavior Map

- Character behavior is split between C++ and Blueprint. C++ owns locomotion state refresh, movement, mantling/ragdoll/roll actions, input callbacks, and camera handoff. `B_Als_Character` owns overlay presentation: it links overlay animation layers and attaches/clears static or skeletal overlay objects when overlay/mantle/ragdoll state changes.
- Animation behavior is layered. `AB_Als` is the main instance, linked AnimBPs split locomotion, grounded state, stance state, layering, ragdoll, head/view, and overlay output. The monolithic AnimBP contains the combined version of these systems for reference.
- Overlay behavior is gameplay-tag driven. `B_Als_Character` switches on overlay gameplay tags for visible objects; overlay AnimBPs provide matching pose layers and weapon/object state machines.
- Data assets under `Data/` drive tuning used by C++ and AnimBP graphs. Do not infer tuning values from asset names; read properties.

## Remaining Gaps

Animation sequence internals, curve values, montage notify payloads, Data Asset defaults, and Control Rig/RigVM internals require focused UE-MCP property reads or Editor inspection.
