# Stealth ALS Prototype Demo - Implementation Guide

## Purpose

This document translates [STEALTH_DEV_DEMO_GAMEPLAY.md](STEALTH_DEV_DEMO_GAMEPLAY.md), Linear TH-507, and the local ALS Refactored documentation into a concrete implementation guide for the first playable stealth dev demo.

It is a guide for project-side implementation. It does not authorize edits to ALS Refactored runtime source, plugin config, or plugin assets.

## Source Context

- Gameplay design: [STEALTH_DEV_DEMO_GAMEPLAY.md](STEALTH_DEV_DEMO_GAMEPLAY.md)
- Linear TH-510: implementation guide request.
- Linear TH-507: merged ALS/ECS architecture.
- Linear TH-506: minimal stealth ECS system.
- ALS plugin docs:
  - `Plugins/ALS-Refactored/SYSTEM_MAP.md`
  - `Plugins/ALS-Refactored/AGENTS.md`
  - `Plugins/ALS-Refactored/Content/AGENTS.md`

## Scope

Build a small playable prototype level that proves:

```text
ALS movement facts
  -> stealth ECS emissions
  -> guard perception
  -> suspicion / alert behavior
  -> objective completion
  -> extraction
  -> debug explanation
```

The first implementation should prefer obvious, inspectable behavior over general frameworks.

## File And Ownership Rules

Allowed implementation areas:

- `Source/echelon_05/`
- project `Content/` for prototype maps, Blueprints, widgets, and data
- project `Config/` only if needed for project-owned inputs/settings
- `Docs/demo/`

Do not edit unless explicitly authorized:

- `Plugins/ALS-Refactored/Source/`
- `Plugins/ALS-Refactored/Content/`
- `Plugins/ALS-Refactored/Config/`

Documentation under `Plugins/ALS-Refactored` may be referenced as evidence. Runtime/plugin changes should remain off-limits to preserve the ALS upgrade path.

## ALS/ECS Boundary

ALS owns character body behavior:

- stance tags: standing, crouching
- gait tags: walking, running, sprinting
- locomotion mode: grounded, in-air
- locomotion action: mantling, rolling, ragdolling, getting-up
- speed, velocity, input/moving facts
- view rotation and yaw speed
- camera view info when using `UAlsCameraComponent`
- animation timing hooks such as footstep notifies

The stealth ECS owns game simulation:

- visibility values
- sound/noise values
- perception events
- suspicion and guard memory
- guard intent/state
- alarm state
- objective state
- extraction state
- debug display state

The adapter is the boundary. It reads ALS facts and writes ECS-facing state. ECS may request high-level movement/presentation intent, but it must not mutate ALS animation internals.

## Verified ALS Facts To Use

From `Plugins/ALS-Refactored/SYSTEM_MAP.md`:

- `AAlsCharacter` exposes desired/resolved view mode, rotation mode, stance, gait, overlay mode, locomotion mode, locomotion action, input direction, `FAlsViewState`, and `FAlsLocomotionState`.
- `FAlsLocomotionState` includes speed, velocity, moving, has-input, input yaw, velocity yaw, and target yaw fields.
- `FAlsViewState` includes rotation and yaw speed.
- `UAlsCharacterMovementComponent` exposes gait amount and ALS movement settings state.
- `UAlsAnimNotify_FootstepEffects` is an animation-timed audiovisual footstep hook, not an ECS sound event by itself.
- `UAlsCameraComponent` can provide camera view info and shoulder state.

Important corrections:

- Do not treat prone as built-in ALS support.
- Do not treat idle as an ALS gait; derive idle from speed/moving.
- Do not treat surface as a persistent ALS character state; derive it from footstep trace/effect context or project-side material queries.
- Do not use ALS collision trace names such as `ECC_Visibility` as stealth visibility concepts.

## Proposed Project-Side Shape

Keep the first pass small. Native C++ should own reusable simulation state and core logic. Blueprints can author level-specific actors, visualization, and tuning where practical.

Suggested C++ folders:

```text
Source/echelon_05/Stealth/
  Components/
  Systems/
  Actors/
  UI/
```

Suggested content folders:

```text
Content/Stealth/
  Blueprints/
  Maps/
  UI/
  Data/
```

The names can change during implementation, but keep prototype content under `/Game/Stealth`.

## ECS Component MVP

These can begin as simple `USTRUCT(BlueprintType)` records, component classes, or a lightweight world subsystem store. Do not build a general-purpose ECS framework before the demo loop works.

### Player / Movement

```cpp
FStealthMovementState
- Stance: Standing | Crouching
- Locomotion: Idle | Walk | Run | Sprint
- Speed
- Velocity
- bHasInput
- bMoving
- Surface: Unknown | Soft | Normal | Metal | Water
```

```cpp
FStealthViewState
- ViewRotation
- ViewDirection
- ViewYawSpeed
- RotationMode
- ViewMode
- bRightShoulder
```

```cpp
FStealthBodyState
- bGrounded
- bAirborne
- LocomotionAction: None | Mantling | Rolling | Ragdolling | GettingUp
```

### Emissions

```cpp
FVisibilityEmitter
- CurrentVisibility
- LightExposure
- SilhouetteExposure
- StanceMultiplier
- MovementMultiplier
- ActionMultiplier
```

```cpp
FSoundEmitter
- CurrentNoise
- Radius
- LastFootstepTime
- LastDiscreteSoundTime
```

```cpp
FSoundEvent
- Position
- Loudness
- Radius
- Surface
- SourceType: Footstep | Lure | Door | Objective | Fallback
- Time
- Lifetime
```

### Guard / Perception

```cpp
FPerceptionSensor
- VisionRange
- VisionAngleDegrees
- HearingRange
- Acuity
```

```cpp
FSuspicionState
- Target
- Value
- State: Unaware | Curious | Suspicious | Investigating | Alert
- LastKnownPosition
- LastStimulusTime
```

```cpp
FGuardBrain
- Mode: Patrol | Pause | Investigate | Search | Chase | Return
- Intent: None | MoveTo | LookAt | CallAlarm
- IntentTarget
```

```cpp
FPatrolRoute
- Points
- CurrentIndex
- bLoop
```

### Mission

```cpp
FObjectiveState
- Required
- Completed
```

```cpp
FExtractionState
- bAvailable
- bUsed
```

```cpp
FAlarmState
- Level
- bActive
- Reason
```

## System MVP And Run Order

1. **ALS Snapshot Adapter**
   - Samples player ALS state after ALS character refresh.
   - Writes movement/view/body facts.

2. **Interaction Probe**
   - Uses view/camera direction to find nearby interactables.

3. **Visibility System**
   - Combines stance, movement, action, light volume, and silhouette placeholder values.

4. **Sound System**
   - Computes current noise and consumes discrete sound events.
   - First pass can use fallback distance polling for footsteps until the footstep bridge is proven.

5. **Perception System**
   - Vision: range, cone, line trace, target visibility.
   - Hearing: sound radius/range and loudness falloff.
   - Outputs perception events.

6. **Suspicion System**
   - Converts perception events into suspicion value/state.
   - Decays suspicion when stimuli stop.

7. **Guard Brain System**
   - Maps suspicion to simple patrol/investigate/chase/return intent.

8. **ECS-to-ALS Guard Requests**
   - Optional for first pass. If using ALS-backed guards, map guard intent to desired gait/stance/facing and movement target.

9. **Alarm System**
   - Raises global alarm from alert guard/camera events.

10. **Objective / Extraction Systems**
   - Completes objective.
   - Unlocks extraction.
   - Completes mission when extraction is used.

11. **Debug Overlay**
   - Displays raw ALS facts plus derived stealth state.

## ALS Snapshot Adapter Contract

Inputs from ALS player actor:

- stance tag
- gait tag
- locomotion mode tag
- locomotion action tag
- rotation mode tag
- view mode tag
- speed
- velocity
- moving
- has input
- input direction
- view rotation
- view yaw speed
- optional gait amount
- optional ALS camera view/shoulder facts

Normalized outputs:

| ALS fact | ECS output |
|----------|------------|
| Standing/Crouching | `FStealthMovementState.Stance` |
| Walking/Running/Sprinting | `FStealthMovementState.Locomotion` |
| `bMoving == false` or speed near zero | `Locomotion = Idle` |
| Grounded/InAir | `FStealthBodyState.bGrounded/bAirborne` |
| Mantling/Rolling/Ragdolling/GettingUp | `FStealthBodyState.LocomotionAction` |
| View rotation/yaw speed | `FStealthViewState` |
| Speed/velocity/input | movement debug and emission formulas |

Adapter rules:

- Sample after ALS has refreshed for the frame.
- Do not compute stealth consequences in the adapter.
- Do not store raw ALS tags outside adapter/debug unless needed for diagnostics.
- Treat unmapped tags as safe defaults plus debug warnings.
- Queue animation-notify sound bridge events separately from the normal tick sample.

## Footstep And Sound Bridge

First prototype path:

1. Use continuous/fallback noise derived from locomotion and speed.
2. Add simple discrete sound emitters for lure/objective interactions.
3. After UE-MCP readiness, evaluate whether a project-side Blueprint/C++ bridge can receive animation-timed footstep events without editing plugin assets.

Possible bridge options:

- Add a project-specific notify to project-owned animation assets later.
- Subclass or wrap footstep notify behavior only if project-owned assets reference it.
- Listen for spawned audio/effect events only if there is a reliable hook.
- Keep distance polling as fallback for demo acceptance.

Do not put stealth formulas inside ALS animation assets.

## Demo Level Actor Setup

Create prototype assets under `Content/Stealth`.

Required actors:

- Player start using ALS player character.
- One guard actor, initially non-ALS or ALS-backed depending on implementation readiness.
- Patrol route actor with 2-4 points.
- Light exposure volumes:
  - lit route volume
  - shadow route volume
- Lure/noise actor.
- Objective actor.
- Extraction actor.
- Level director / stealth simulation manager.
- Debug overlay widget or debug draw actor.

Recommended map:

```text
/Game/Stealth/Maps/L_StealthAls_Prototype
```

## Initial Tuning Table

These are intentionally readable starting values, not final balance.

| Value | Starting point |
|-------|----------------|
| Base visibility | 0.5 |
| Standing multiplier | 1.0 |
| Crouching multiplier | 0.45 |
| Idle movement multiplier | 0.65 |
| Walk movement multiplier | 1.0 |
| Run movement multiplier | 1.35 |
| Sprint movement multiplier | 1.75 |
| Mantle/action multiplier | 1.5 |
| Shadow light exposure | 0.15 |
| Lit light exposure | 0.85 |
| Walk noise radius | 450 cm |
| Run noise radius | 900 cm |
| Sprint noise radius | 1400 cm |
| Crouch noise multiplier | 0.45 |
| Suspicion curious threshold | 20 |
| Suspicion suspicious threshold | 45 |
| Suspicion investigating threshold | 70 |
| Alert threshold | 100 |
| Suspicion decay | 8/sec after grace |

Tune relationships before exact numbers.

## Debug Overlay Requirements

The debug overlay must show both causes and consequences.

ALS facts:

- stance tag
- gait tag
- speed
- locomotion mode
- locomotion action
- rotation mode
- moving / has input
- view yaw speed
- last footstep/sound bridge timestamp if available

Stealth consequences:

- light exposure
- current visibility
- current noise radius
- active sound events
- guard state
- guard suspicion value
- last known player position
- alarm level
- objective state
- extraction availability

Debug draw:

- guard vision cone
- hearing/sound radius
- patrol points
- lit/shadow volumes
- interaction target/probe if useful

## Test Scenarios

### Scenario A: Safe Sneak

Steps:

1. Start level.
2. Crouch-walk through the shadow route.
3. Reach objective.
4. Extract.

Expected:

- Visibility remains low.
- Guard does not reach alert.
- Objective completes.
- Extraction succeeds.

### Scenario B: Intentional Detection

Steps:

1. Stand or sprint through lit route in guard view.
2. Remain visible long enough for suspicion to rise.

Expected:

- Visibility/noise increase.
- Guard suspicion rises to alert.
- Guard enters chase/alert behavior.
- Overlay shows cause.

### Scenario C: Lure

Steps:

1. Trigger lure/noise actor.
2. Wait for guard to investigate.
3. Move through opened path.

Expected:

- Sound event appears.
- Guard moves/looks toward sound.
- Player can exploit path.

### Scenario D: Extraction Gate

Steps:

1. Go to extraction before objective.
2. Complete objective.
3. Return to extraction.

Expected:

- Extraction unavailable before objective.
- Extraction available after objective.
- Mission completes on extraction.

## UE-MCP Dependency

Blueprint and level authoring work should wait for TH-511 readiness or explicitly run the same smoke test first.

Minimum readiness needed before gameplay Blueprint work:

- `project(action="get_status")` succeeds.
- Known ALS Blueprint package paths can be read.
- Disposable Blueprint can be created, mutated, compiled, and read back.
- Disposable actor can be placed or verified in a level.
- Tool gaps are documented.

## Known Gaps And Deferrals

- Host stealth ECS implementation does not exist yet.
- Exact ALS Blueprint defaults remain UE-MCP/Editor inspection tasks.
- ALS sample BT/BB semantics remain binary asset gaps.
- Footstep event bridge strategy is not proven yet.
- Guard can start as simple project-owned AI before ALS-backed guard presentation.
- Multiplayer replication is deferred.
- Final art and production mission design are deferred.

## Milestone Plan

### Milestone 1: Foundation And Adapter

- Add minimal stealth state structures.
- Link ALS player actor to stealth player entity/state.
- Sample ALS stance/gait/speed/body/view facts.
- Show raw adapter output in debug.

Done when ALS crouch/sprint/action changes update debug live.

### Milestone 2: Emissions

- Add light/shadow volumes.
- Compute visibility.
- Compute continuous noise.
- Add simple sound event actor.

Done when crouch-walking in shadow is lower visibility/noise than sprinting in light.

### Milestone 3: One Guard Perception

- Add guard sensor.
- Add vision cone and hearing checks.
- Add suspicion rise/decay.

Done when player can be partially spotted, recover, or become alerted.

### Milestone 4: Guard Intent

- Add patrol route.
- Add investigate sound/last known position behavior.
- Add alert/chase or alert marker.

Done when lure changes guard position and opens the route.

### Milestone 5: Mission Loop

- Add objective.
- Add extraction.
- Add win/compromised states.

Done when the player can infiltrate, avoid/lure guard, complete objective, and extract.

## Implementation Acceptance

This guide is ready when:

- Every gameplay beat from the gameplay doc maps to a system or actor responsibility.
- ALS -> ECS and ECS -> ALS values are explicit.
- Agents know which paths are allowed and which are off-limits.
- The debug overlay is specified as part of the deliverable.
- UE-MCP readiness is identified as a gate for Blueprint/level work.
