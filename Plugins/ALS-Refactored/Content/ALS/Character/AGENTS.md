# AGENTS.md — Plugins/ALS-Refactored/Content/ALS/Character/

Packaged ALS character and animation Blueprint assets. Files are binary `*.uasset`; use UE-MCP package paths under `/ALS/ALS/Character/...` for inspection.

## Blueprint Inventory

| Asset | UE-MCP notes |
|------|--------------|
| `B_Als_Character.uasset` | Blueprint class `B_Als_Character`, parent `AlsCharacterExample`; adds `OverlaySkeletalMesh` and `OverlayStaticMesh`. Graphs handle overlay object attachment, overlay linked animation layers, mantling settings selection, and mantle/ragdoll overlay refresh. |
| `AB_Als.uasset` | Main `AlsAnimationInstance` AnimBP; `AnimGraph`, `Overlay`, empty `EventGraph`. |
| `AB_Als_Monolithic.uasset` | Monolithic `AlsAnimationInstance` AnimBP; large graph surface combining locomotion, grounded/air states, stance states, layering, ragdolling, head, and overlay graphs. |

## `B_Als_Character` Behavior

`B_Als_Character` is the Blueprint layer over `AAlsCharacterExample`. C++ owns core locomotion, input callbacks, mantling, ragdolling, rolling, and camera handoff; this Blueprint primarily handles presentation choices that are easier to author in assets.

- On `BeginPlay`, it calls `RefreshOverlayObject`, so the visible carried/held overlay object matches the initial `OverlayMode`.
- On `OnOverlayModeChanged`, it first calls `RefreshOverlayLinkedAnimationLayer`, then calls `RefreshOverlayObject`.
- `RefreshOverlayLinkedAnimationLayer` reads `OverlayMode`, looks it up in `OverlayAnimationInstanceClasses`, stores `OverlayAnimationInstanceClass`, then calls `Link Anim Class Layers` on the character mesh. If the lookup has no valid class, it links with an empty class path, effectively clearing the overlay linked layer.
- `RefreshOverlayObject` switches on gameplay tag `OverlayMode`. Rifle, pistol, bow, torch, binoculars, box, and barrel cases call `AttachOverlayObject`; default calls `ClearOverlayObject`.
- `AttachOverlayObject` sets both possible overlay renderers: `OverlayStaticMesh` gets `NewStaticMesh`; `OverlaySkeletalMesh` gets `NewSkeletalMesh` and `NewAnimationClass`. Both attach to the main mesh using a resolved socket name.
- Socket selection prefers the explicit `SocketName`, unless `bUseGunBoneForOverlayObjects` is true. In that case it selects left or right gun virtual bone based on `bUseLeftGunBone`.
- `ClearOverlayObject` clears the static mesh, skeletal mesh, and skeletal overlay anim class.
- On mantling/ragdolling start, it clears overlay objects; on mantling/ragdolling end, it refreshes them again. Mantling start only clears if the mantling type is not the excluded enum value checked by the graph.
- `SelectMantlingSettings` first switches on `EAlsMantlingType`; low mantles further switch by `OverlayMode`, giving different settings for injured, hands tied, weapon/binoculars, box, and other overlays.

Behavior gap: the exact asset values passed into `AttachOverlayObject`, the `OverlayAnimationInstanceClasses` map contents, and mantling settings assets need focused property reads or Editor inspection.

## Linked Animation Layers

Assets under `AnimationInstances/` subclass `AlsLinkedAnimationInstance` unless noted:

- `AB_Als_Locomotion` — locomotion state machine, grounded/fall/jump/land states.
- `AB_Als_Grounded` — grounded stance state machine, roll, standing/crouching transitions.
- `Stances/AB_Als_Standing` — standing movement, start/stop, pivot, rotate, movement direction states.
- `Stances/AB_Als_Crouching` — crouching idle/move/stop/rotate and movement direction states.
- `AB_Als_Layering` — large layering `AnimGraph`.
- `AB_Als_Ragdolling`, `AB_Als_Head`, `AB_Als_View` — small linked AnimGraphs; `AB_Als_View` reports class name `AB_Als_Head` through the bridge, so verify before editing.

## Animation Behavior Shape

The animation assets split a large ALS animation graph into linked layers:

- `AB_Als` is the main `UAlsAnimationInstance` graph. It owns the top-level `AnimGraph` and calls into an `Overlay` layer.
- `AB_Als_Monolithic` contains the same major systems in one asset: locomotion, grounded/air flow, standing/crouching stances, layering, ragdolling, head, and overlay.
- `AB_Als_Locomotion` routes between grounded and in-air behavior. Its graphs include grounded, fall, jump, flail, land, land movement, and jump foot states.
- `AB_Als_Grounded` selects between standing, crouching, stance transitions, and roll.
- `AB_Als_Standing` handles standing idle/move/stop, turn/rotate, walk/run/start/pivot detail, and directional movement states.
- `AB_Als_Crouching` mirrors the crouched version of idle/move/stop/rotate and directional movement.
- `AB_Als_Layering` is the large pose layering layer used to combine locomotion with upper-body and overlay poses.
- `AB_Als_Ragdolling`, `AB_Als_Head`, and `AB_Als_View` are smaller layers for focused pose output.

Do not treat node counts as behavior proof. They identify where behavior lives; use `read_graph_summary` on the specific graph before changing or documenting detailed transitions.

## Overlay Animation Blueprints

Overlay assets live under `AnimationInstances/Overlays/`:

- `AB_Als_Default`, `AB_Als_Masculine`, `AB_Als_Feminine` — `Overlay` graphs with 19 nodes.
- `AB_Als_Rifle`, `AB_Als_PistolOneHanded`, `AB_Als_PistolTwoHanded`, `AB_Als_Bow` — overlay state machines with relaxed / aiming / ready states.
- `AB_Als_Torch`, `AB_Als_Binoculars`, `AB_Als_Box`, `AB_Als_Barrel`, `AB_Als_HandsTied`, `AB_Als_Injured` — overlay graphs with no event logic observed.

Overlay behavior shape:

- Default / masculine / feminine overlays provide baseline overlay pose layers.
- Weapon overlays (`Rifle`, `PistolOneHanded`, `PistolTwoHanded`, `Bow`) add overlay state machines. The observed states are `Relaxed`, `Aiming`, and `Ready`, with transition graphs between them.
- Object/status overlays (`Torch`, `Binoculars`, `Box`, `Barrel`, `HandsTied`, `Injured`) provide overlay pose graphs without observed EventGraph logic.
- `OverlayObjects/Bow/AB_Als_Bow` is a separate AnimBP for the bow object itself, not the character overlay layer.

## Agent Rules

1. Prefer `blueprint(action="list_graphs")` and `read_graph_summary` before full graph reads; large AnimBPs can produce very large outputs.
2. Do not infer animation behavior from filenames alone; cite UE-MCP graph evidence or mark a gap.
3. For exact pin defaults, linked layer class maps, settings assets, and Control Rig/RigVM semantics, perform focused property/graph reads or inspect in Editor.
