# AGENTS.md — Plugins/ALS-Refactored/Content/ALSExtras/

Sample / optional ALS content. Files are binary `*.uasset` / `*.umap`; use UE-MCP package paths under `/ALS/ALSExtras/...` for Blueprint inspection.

## Blueprint Inventory

| Area | Assets | UE-MCP notes |
|------|--------|--------------|
| Core | `Core/B_Als_PlayerController`, `Core/B_Als_GameMode`, `Core/AB_Als_PostProcessing` | Player controller creates HUD / overlay menu widgets, adds ALSExtras input mapping, toggles UI, overlay menu, and slomo. Game mode is a mostly data-default Blueprint. Post-processing AnimBP has a small `AnimGraph`. |
| AI | `AI/AIC_Als`, `AI/B_Als_AICharacter`, `AI/BTT_Als_FocusPlayer`, `AI/BTT_Als_GetRandomLocationInRadius` | AI controller subclasses `AlsAIController`; AI character subclasses `B_Als_Character`. Focus task sets focus to player pawn. Random-location task writes a reachable nav point into a blackboard vector. |
| Environment | `Environment/B_Als_StaticObject`, `Environment/B_Als_MovingObject`, `Environment/B_Als_RotatingObject` | Static object construction script applies mesh/material and material color/tiling params. Moving object moves mesh along a spline using server time plus ping compensation. Rotating object derives rotation from server time plus ping compensation. |
| Editor | `Editor/B_Als_SkeletonAssetActions` | Asset action utility with `SetupAlsSkeleton` graph. |
| UI | `UI/W_Als_Hud`, `UI/W_Als_OverlayModeMenu`, `UI/W_Als_OverlayModeOption` | HUD refreshes debug/slomo key UI. Overlay menu selects previous/next overlay mode and approves selection. Option widget sets text and selected state. |

## Runtime Behavior

### Player Controller And UI

`B_Als_PlayerController` owns the optional sample UI flow:

- On `BeginPlay`, it checks `Is Local Controller`. Only local controllers create UI and add input mappings.
- It calls `Set Input Mode Game Only`, adds the ALSExtras Enhanced Input mapping context, creates `W_Als_Hud`, and adds it to the viewport.
- It also creates/stores `OverlayModeMenuWidget`, but menu visibility is managed later instead of always staying on screen.
- `IA_Als_OverlayModeMenu` toggles the overlay menu. When the menu is opened, it adds `W_Als_OverlayModeMenu` to the viewport; when approved/closed, it removes it from parent.
- `IA_Als_NextOverlayMode` and `IA_Als_PreviousOverlayMode` only act when the overlay menu widget is in the viewport, then call the menu widget's selection functions.
- `IA_Als_ToggleUI` flips `bUIVisible`.
- `IA_Als_Slomo` flips `bSlomoActive`, selects a time-dilation value, and calls `SetGlobalTimeDilation`.

`W_Als_OverlayModeMenu` handles the selection workflow. Its non-empty graphs select previous/next overlay mode, approve the selected mode, react to selected-overlay changes, and run the menu EventGraph. `W_Als_OverlayModeOption` is a row-style widget: `SetText` updates display text and `SetSelected` updates selected presentation. `W_Als_Hud` refreshes debug key labels/colors for ALS debug channels and slomo.

### AI Sample

`AIC_Als` subclasses `AAlsAIController`; the C++ parent runs its assigned behavior tree on possession. `B_Als_AICharacter` subclasses `B_Als_Character`, so AI pawns reuse the same overlay object and linked animation layer behavior.

Readable task Blueprints:

- `BTT_Als_FocusPlayer`: on `Receive Execute AI`, gets player pawn, calls `SetFocus` on the owner controller, then finishes execute.
- `BTT_Als_GetRandomLocationInRadius`: on `Receive Execute AI`, gets the controlled pawn location, calls `Get Random Reachable Point in Radius` using `Radius`, writes the result to blackboard key `LocationKey` when successful, then finishes execute with success/failure.

Behavior gap: `BT_Als` tree ordering and `BB_Als` key schema were not exposed by the Blueprint tool. Do not describe higher-level AI behavior beyond the readable tasks without BT/BB inspection.

### Environment Actors

`B_Als_StaticObject` is a configurable prop Blueprint:

- Construction script sets the `StaticMesh` component mesh from a `Mesh` variable.
- It sets the material from a `Material` variable.
- It pushes material parameters for top/side colors and tiling into the mesh materials.
- It does not tick.

`B_Als_MovingObject` subclasses `B_Als_StaticObject` and adds `MovementSpline`:

- EventGraph calls `RefreshMeshLocation`.
- `RefreshMeshLocation` validates `GameState`, reads server world time plus first-player ping, divides by movement `Duration`, and stores `MovementTimeRatio`.
- It uses floor/modulo math to alternate `bInverseMovement`, evaluates `MovementCurve`, samples `MovementSpline` at a time value, and sets the static mesh world location.
- This makes movement deterministic from replicated/server time rather than purely local tick accumulation.

`B_Als_RotatingObject` subclasses `B_Als_StaticObject`:

- EventGraph calls `RefreshActorRotation`.
- `RefreshActorRotation` validates `GameState`, reads server world time plus first-player ping, scales `RotationSpeed` by that time, and calls `Set Actor Rotation`.
- Like the mover, rotation is derived from shared time rather than accumulated local-only state.

### Editor Utility

`B_Als_SkeletonAssetActions` is an `AssetActionUtility` with a `SetupAlsSkeleton` graph. Treat it as an editor authoring helper for skeleton setup; inspect the graph before changing skeleton modification behavior.

## Levels And World Partition

- `Levels/L_Als_Playground.umap` and `Levels/L_Als_Grid.umap` are plugin sample levels.
- Their payloads live under `../__ExternalActors__/ALSExtras/Levels/...` and `../__ExternalObjects__/ALSExtras/Levels/...`.
- Repo-root `/Game/L_Als_Playground` is a separate copy; do not treat it as plugin content unless the user explicitly expands scope.

## Remaining Gaps

Behavior Tree and Blackboard assets (`AI/BT_Als.uasset`, `AI/BB_Als.uasset`) are present, but graph/key semantics were not read by the Blueprint tool. Use an Editor inspection or a dedicated BT/BB-capable tool before documenting their behavior.
