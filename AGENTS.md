# AGENTS.md - echelon_05 root

Root operating guide for agents working in this Unreal Engine 5.7 project.

## Start Here

For general repo orientation, read:

1. `README.md`
2. this file

For Linear TH-513 / stealth ALS prototype implementation, read in this order:

1. `Docs/Implementation/TH513_AGENT_RUNBOOK.md`
2. `Docs/Implementation/STEALTH_ALS_PROTOTYPE_TASKS.md`
3. `Docs/demo/STEALTH_ALS_PROTOTYPE_IMPL.md`
4. `Docs/demo/STEALTH_DEV_DEMO_GAMEPLAY.md`
5. `Docs/Implementation/UE_MCP_READINESS.md` before Blueprint or level authoring

Do not create Linear subtasks or child issues for TH-513 unless explicitly requested. The 10 tasks in the repo doc are execution phases inside the single TH-513 issue.

## Current Workstreams

### TH-513 - Stealth ALS Prototype Demo

Implement one playable prototype loop:

```text
ALS player movement facts
  -> project-side stealth state
  -> visibility and sound emissions
  -> one guard perception and suspicion
  -> simple patrol / investigate / alert behavior
  -> objective completion
  -> extraction
  -> debug explanation
```

Default first-pass decisions:

- C++ owns reusable stealth simulation state and core logic.
- Blueprints may own prototype actor setup, map placement, tuning data, simple UI, and debug presentation.
- Guard can start as a simple project-owned non-ALS actor.
- Footsteps use fallback distance/time polling first.
- Prototype content lives under `/Game/Stealth`.
- Project C++ should live under `Source/echelon_05/Stealth`.

### TH-505 / ALS Locomotion Documentation

TH-505 locomotion / ALS navigation work is scoped to plugin docs and paths under `Plugins/ALS-Refactored/` only. Do not treat host game `Source/`, root `Config/`, or repo-root `Content/` as ALS scope unless the task explicitly expands scope.

## ALS Refactored Boundary

`Plugins/ALS-Refactored` is a frozen upstream plugin dependency by default.

Do not edit unless explicitly authorized:

- `Plugins/ALS-Refactored/Source/`
- `Plugins/ALS-Refactored/Content/`
- `Plugins/ALS-Refactored/Config/`

Documentation under `Plugins/ALS-Refactored` may be referenced as evidence. Documentation edits inside the plugin should be factual, evidence-based, and clearly marked with gaps. Documentation is not permission to alter ALS runtime source, assets, or config.

If a feature appears to require ALS plugin edits, stop and report the tradeoff:

- keep ALS frozen and build project-side wrappers/adapters
- fork/modify ALS and accept upgrade drift

## Canonical ALS Docs

- `Plugins/ALS-Refactored/SYSTEM_MAP.md` - plugin system map, evidence legend, subsystems, Mermaid diagram, gaps.
- `Plugins/ALS-Refactored/AGENTS.md` - module map and upgrade/read-first notes for the plugin subtree.
- `Plugins/ALS-Refactored/Content/AGENTS.md` - packaged `*.uasset` / `*.umap` layout under the plugin only.

For ALS plugin Blueprints, use package paths under `/ALS`, for example:

```text
/ALS/ALS/Character/B_Als_Character
```

Native `asset(action="list")` may not enumerate plugin mounts, but `blueprint(...)` can read known package paths.

## UE-MCP Rules

When using UE-MCP:

1. Start with `project(action="get_status")`.
2. Confirm the editor is connected to `echelon_05`.
3. Use Unreal package paths, not filesystem paths.
4. Read assets before mutating.
5. Compile after Blueprint mutations.
6. Save and verify placement/readback after level mutations.

Known project map/package facts:

- Repo-root playground map: `/Game/L_Als_Playground`
- ALS plugin content mount: `/ALS`
- Recommended TH-513 prototype map: `/Game/Stealth/Maps/L_StealthAls_Prototype`

## Git And Unreal Assets

Git LFS is configured for Unreal assets via `.gitattributes`, including:

- `*.uasset`
- `*.umap`
- common mesh, image, audio, and related binary source formats

`Content/__ExternalActors__/...` files are World Partition / One File Per Actor payloads. Commit them with their matching intentional `.umap` changes instead of globally ignoring them.

`.gitignore` excludes Unreal-generated and local-only folders such as:

- `Binaries/`
- `Intermediate/`
- `Saved/`
- `DerivedDataCache/`
- plugin build folders
- `.cursor/`
- IDE folders such as `.vs/`

Do not commit generated build products unless a task explicitly requires a checked-in artifact.

## Build / Launch

Use the Node helpers from the repo root:

```powershell
npm run build
npm run launch
npm run rebuild
```

`package.json` and `scripts/` provide build, launch, clean, log cleanup, rebuild, and dev flows using the UE 5.7 engine path. The scripts include WSL-aware PowerShell bridging.

## Stop Conditions

Stop and report instead of guessing if:

- UE-MCP cannot connect to the editor.
- the project does not build before gameplay work begins.
- a required implementation path appears to need edits under `Plugins/ALS-Refactored`.
- ALS animation assets or ALS notifies would need modification.
- it is unclear whether a binary asset belongs with an intentional map change.
- a task requires broad architecture not described in the docs.

## Learned User Preferences

- For TH-505 / ALS locomotion documentation, keep scope limited to `Plugins/ALS-Refactored` unless the user explicitly expands it.
- For TH-513, keep Linear as one implementation issue; use repo docs as execution phases, not Linear subtasks.
- For UE-MCP Blueprint readiness work, structural create/compile/place smoke tests are not enough; include visible runtime behavior that exercises graph wiring and variables.
- For stealth prototype work, prioritize in-editor legibility and observable feedback: route markers, guard bodies, patrol points, debug HUD, and debug draw should be visible enough to validate without opening assets.
- For UE-MCP reads of ALS Blueprints or live PIE state, serialize calls one at a time; batched reads are more likely to time out or fail.
- For stealth guard behavior, favor Splinter Cell-style armed containment: maintain LOS and standoff, backpedal/strafe when crowded, and avoid sprinting directly into the player.
- For stealth hearing design, keep sound reactions value-driven through suspicion/evidence math; avoid separate ad hoc pause or investigate timers that bypass suspicion state.
- "Prototype" does not mean shortcuts are acceptable; implement features correctly even in early phases. Do not assume the user tolerates quick hacks because the work is described as a prototype.
- Stealth HUD main panel must show only player-facing gameplay info (detection bar, VIS/NSE meters, stance, mission status); debug/tuning readouts (event counts, light-level percentages, raw numbers) must be gated behind `stealth.DebugDraw`.

## Learned Workspace Facts

- `/Game/Stealth` is the canonical TH-513 content root; `/Game/_Dev/StealthDemo` has been migrated away, and disposable MCP smoke examples should use `/Game/MCP_Smoke`.
- ALS-backed stealth guard work should mirror the ALSExtras pawn/controller split project-side while keeping stealth BT/BB semantics project-owned; `BT_Als` behavior is only a sample pattern.
- ALS-backed stealth guards need valid ALS `Settings` and `MovementSettings` such as `CS_Als_Default` and `MS_Als_Normal`; missing settings make ALS skip input refresh and can look like shuffling in place.
- UE-MCP `level(save)` and `asset(move_folder)` have been unreliable in this workspace; verify with readback/disk state and use editor save APIs or per-asset moves/duplicates when native success contradicts results.
- `/Game/Stealth/Maps/L_StealthAls_Prototype` is moving toward a single ALS-backed guard setup; avoid keeping the legacy baseline `Stealth_Guard` alongside `Stealth_AlsGuard` unless explicitly comparing behavior.
- Stealth patrol authoring now expects patrol points to have their own editable transforms/gizmos rather than hidden array-only offsets.
- ALS-backed stealth guard presentation should stay rifle-ready during patrol and reserve aiming/combat movement for alert states; overlay objects and AnimBP layers should be mirrored project-side without editing `/ALS`.
- Rendering↔perception contract: what the player sees visually (Lumen + PPV) must match what `StealthSimulationSubsystem` perceives; a guard must never detect a player in an area that renders pitch-black. The preferred fix is PPV-aware ambient derivation in the subsystem, not manual calibration of `SceneLightAmbientExposure`.
- `StealthSimulationSubsystem` light sampling is renderer-independent (direct line-traced scene-light intensity); `SceneLightAmbientExposure=0.06` is a hardcoded floor that does not track PPV exposure bias or indirect-intensity overrides — it needs to read active PPV settings to stay in sync.
- `L_StealthLightLab` uses `PPV_StealthNoir` (infinite extent, manual exposure, indirect=0, film S-curve), DirectionalLight at intensity 3 with no indirect/atmospheric, and `SkyLight_AmbientFloor` at intensity 0.04; root cause of prior overexposure was `SkyAtmosphere` + DirectionalLight at intensity 11 with no PostProcessVolume.
- `PlayerStart_LightLab` should be placed in the ShadowBay (X≈0) for meaningful stealth testing; the SunBay placement (X≈−2200) shares the zone with the guard and bypasses the visibility gradient.

