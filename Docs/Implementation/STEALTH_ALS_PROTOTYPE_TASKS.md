# Stealth ALS Prototype Demo - Agent Task Breakdown

## Purpose

This document turns the stealth ALS gameplay design, implementation guide, and UE-MCP readiness results into agent-sized implementation tasks.

The intent is to be prescriptive about boundaries, dependencies, acceptance criteria, and verification, while leaving implementation details open where a local agent can make a better codebase-aware choice.

## Source Context

- Linear TH-512: autonomous agent task breakdown request.
- Linear TH-507: merged ALS/ECS architecture.
- Linear TH-506: minimal stealth ECS design.
- Gameplay design: `Docs/demo/STEALTH_DEV_DEMO_GAMEPLAY.md`.
- Implementation guide: `Docs/demo/STEALTH_ALS_PROTOTYPE_IMPL.md`.
- UE-MCP readiness: `Docs/Implementation/UE_MCP_READINESS.md`.
- ALS plugin docs:
  - `Plugins/ALS-Refactored/SYSTEM_MAP.md`
  - `Plugins/ALS-Refactored/AGENTS.md`
  - `Plugins/ALS-Refactored/Content/AGENTS.md`

## Global Rules

Allowed implementation areas:

- `Source/echelon_05/`
- project `Content/`, preferably isolated under `/Game/Stealth`
- project `Config/` only when a project-owned setting is required
- `Docs/demo/` and `Docs/Implementation/`

Do not edit unless explicitly authorized:

- `Plugins/ALS-Refactored/Source/`
- `Plugins/ALS-Refactored/Content/`
- `Plugins/ALS-Refactored/Config/`

Blueprint and level tasks must follow the UE-MCP readiness discipline:

- Start with `project(action="get_status")`.
- Use Unreal package paths, not filesystem paths.
- Read before mutating.
- Compile after Blueprint mutations.
- Save and verify placement/readback after level mutations.

## Task Sizing

Each task should be small enough for one autonomous agent pass. A task may choose its local implementation strategy, but it must preserve the acceptance criteria and verification contract. If an agent discovers that the task needs a broader architecture change, it should stop, document the finding, and propose the next task rather than silently expanding scope.

## Task 1: Foundation Stealth Types And Runtime Container

### Goal

Create the minimal project-side runtime state needed for the stealth prototype loop: movement, view, body, emissions, perception, suspicion, guard brain, objective, extraction, alarm, and sound events.

### Allowed Files / Scope

- `Source/echelon_05/Stealth/**`
- `Source/echelon_05/echelon_05.Build.cs` if module dependencies are required
- Documentation notes in `Docs/Implementation/`

### Dependencies

- `Docs/demo/STEALTH_ALS_PROTOTYPE_IMPL.md`, especially "ECS Component MVP" and "System MVP And Run Order".

### Acceptance Criteria

- Project-owned C++ types exist for the MVP state listed in the implementation guide.
- The design does not introduce a broad general-purpose ECS framework before the demo loop works.
- Types are usable from C++ and exposed to Blueprint only where that helps prototype authoring or debug.
- No ALS plugin files are modified.

### Verification

- Run the normal project build command or Unreal compile path used in this repo.
- Confirm the new types compile cleanly.
- If Blueprint exposure is added, verify the structs/classes appear in the editor or can be referenced by a disposable Blueprint.

### Do-Not-Touch Notes

- Do not edit `Plugins/ALS-Refactored/**`.
- Do not add stealth formulas to ALS animation assets or notifies.

### Expected Output

- A small set of project-owned C++ types/components/subsystems that later tasks can consume.

## Task 2: ALS Snapshot Adapter

### Goal

Link the player ALS character to the stealth runtime state and normalize ALS movement/body/view facts into project-owned stealth state.

### Allowed Files / Scope

- `Source/echelon_05/Stealth/**`
- project-owned player wrapper/component/Blueprint under `/Game/Stealth/**` if needed
- debug documentation updates in `Docs/Implementation/`

### Dependencies

- Task 1.
- `Plugins/ALS-Refactored/SYSTEM_MAP.md`.
- `Docs/demo/STEALTH_ALS_PROTOTYPE_IMPL.md`, especially "ALS Snapshot Adapter Contract".

### Acceptance Criteria

- Adapter reads verified ALS facts such as stance, gait, locomotion mode, locomotion action, speed, velocity, moving, has-input, view rotation, and view yaw speed.
- Adapter outputs normalized stealth movement, view, and body state.
- `Idle` is derived from speed or moving state, not treated as an ALS gait.
- `Prone` is not treated as built-in ALS support.
- Surface/material is left unknown unless a project-side query or event provides it.
- Adapter does not compute visibility, sound, suspicion, alarm, objective, or extraction consequences.

### Verification

- In editor, change player stance/gait/action and verify the adapter state changes live through logs, debug draw, or an interim overlay.
- Verify unmapped tags fall back safely and are visible as diagnostics.

### Do-Not-Touch Notes

- Do not mutate ALS private state or animation graph internals.
- Do not edit ALS plugin source, content, or config.

### Expected Output

- A project-side ALS-to-stealth boundary that future systems can consume without depending on raw ALS tags.

## Task 3: Debug Overlay Foundation

### Goal

Create the first debug surface that shows raw ALS adapter facts and derived stealth state as the prototype grows.

### Allowed Files / Scope

- `Source/echelon_05/Stealth/**`
- `/Game/Stealth/UI/**`
- `/Game/Stealth/Blueprints/**`
- `Docs/Implementation/**`

### Dependencies

- Task 1.
- Task 2 for live ALS facts, though a stub/mock state is acceptable if Task 2 is still in progress.
- `Docs/demo/STEALTH_DEV_DEMO_GAMEPLAY.md`, especially "Readability Rules".

### Acceptance Criteria

- Debug output shows at least stance, gait/derived locomotion, speed, locomotion mode/action, moving/has-input, view yaw speed, visibility, noise radius, guard state, suspicion, objective state, and extraction availability as those values become available.
- Missing systems display clear placeholder or unavailable values rather than hiding fields.
- Debug output is easy to inspect during PIE.

### Verification

- Start the prototype map or playground map.
- Confirm the debug surface updates while the player moves, crouches, stops, and sprints.
- Capture a short note or screenshot path in the task result if practical.

### Do-Not-Touch Notes

- Do not build a polished HUD before the simulation is readable.
- Do not make debug state dependent on ALS plugin asset edits.

### Expected Output

- A live causality panel or debug draw layer that becomes the shared proof surface for later tasks.

## Task 4: Light And Visibility Emissions

### Goal

Compute player visibility from ALS-derived stance/locomotion/body state plus simple project-owned lit/shadow volumes.

### Allowed Files / Scope

- `Source/echelon_05/Stealth/**`
- `/Game/Stealth/Blueprints/**`
- `/Game/Stealth/Data/**`
- `/Game/Stealth/Maps/**` if volumes are placed

### Dependencies

- Task 1.
- Task 2.
- Task 3 for verification display.

### Acceptance Criteria

- Visibility changes with stance: crouching is lower than standing.
- Visibility changes with derived locomotion: idle/walk/run/sprint follow the relationships in the gameplay and implementation docs.
- Lit/shadow volumes affect light exposure.
- Locomotion actions such as mantling or in-air state can raise visibility when those facts are available.
- Debug output shows both raw inputs and final visibility.

### Verification

- In PIE, compare crouch-walking in shadow against standing or sprinting in light.
- Confirm final visibility value and displayed multipliers move in the expected direction.

### Do-Not-Touch Notes

- Do not use ALS collision channel names as stealth visibility concepts.
- Do not add stealth detection logic to ALS.

### Expected Output

- A project-owned visibility emitter path that guard vision can consume later.

## Task 5: Sound Events And Footstep Fallback

### Goal

Create the first sound/noise path: continuous movement noise plus discrete project-owned sound events for lure/objective interactions, with a clear fallback strategy for footsteps.

### Allowed Files / Scope

- `Source/echelon_05/Stealth/**`
- `/Game/Stealth/Blueprints/**`
- `/Game/Stealth/Data/**`
- `/Game/Stealth/Maps/**`

### Dependencies

- Task 1.
- Task 2 for movement facts.
- Task 3 for verification display.

### Acceptance Criteria

- Movement noise radius changes with derived locomotion and crouch state.
- At least one project-owned actor can emit a discrete sound event with position, loudness/radius, type, time, and lifetime.
- Footsteps use fallback distance/time polling unless a project-side animation-timed bridge is proven without plugin edits.
- Surface/material remains optional and may be `Unknown`.
- Debug output shows current noise radius and active/recent sound events.

### Verification

- In PIE, compare crouch-walk, walk, run, and sprint noise radii.
- Trigger the sound/lure actor and verify a sound event appears at the expected position and expires.

### Do-Not-Touch Notes

- Do not put stealth sound formulas inside ALS animation notifies or animation graphs.
- Do not edit ALS plugin assets to add notifies.

### Expected Output

- Sound data that guard hearing can consume, plus a documented path for later footstep bridge refinement.

## Task 6: One Guard Perception And Suspicion

### Goal

Add one guard that can perceive the ALS-driven player through project-owned vision and hearing and convert those stimuli into suspicion state.

### Allowed Files / Scope

- `Source/echelon_05/Stealth/**`
- `/Game/Stealth/Blueprints/**`
- `/Game/Stealth/Maps/**`
- `/Game/Stealth/Data/**`

### Dependencies

- Task 1.
- Task 4 for visibility.
- Task 5 for sound events/noise.
- Task 3 for debug display.

### Acceptance Criteria

- Guard vision uses range, cone, line trace or equivalent occlusion check, and player visibility.
- Guard hearing consumes current/recent sound events or player noise radius.
- Suspicion can rise, decay, and cross at least curious/suspicious/alert thresholds.
- Debug output shows guard state, suspicion value, last known/stimulus position, and reason for the latest suspicion change.

### Verification

- Safe sneak scenario: crouch-walk through shadow and confirm guard does not reach alert.
- Detection scenario: stand or sprint in lit line of sight and confirm suspicion rises to alert.
- Sound scenario: emit lure sound and confirm guard receives a hearing stimulus.

### Do-Not-Touch Notes

- Do not move perception, suspicion, or alert ownership into ALS.
- Do not require production-grade squad AI.

### Expected Output

- The first complete stealth causality chain: ALS movement facts influence visibility/sound, and those values influence guard suspicion.

## Task 7: Patrol, Investigate, Chase, And Return Intent

### Goal

Give the guard simple readable behavior: patrol while unaware, investigate stimuli, chase or alert when suspicion is high, and return when suspicion decays.

### Allowed Files / Scope

- `Source/echelon_05/Stealth/**`
- `/Game/Stealth/Blueprints/**`
- `/Game/Stealth/Maps/**`

### Dependencies

- Task 6.

### Acceptance Criteria

- Guard follows a small patrol route with 2-4 points.
- Hearing or weak visual stimulus can move the guard into investigate/search behavior.
- Alert state produces a visible chase, call-alarm marker, or equivalent pressure state.
- Guard can return to patrol after suspicion decays, unless alert behavior intentionally remains sticky.
- If guard presentation uses ALS, ECS still owns the guard intent and ALS only presents movement/body state.

### Verification

- In PIE, observe patrol, trigger a lure, verify guard investigates the sound location, then verify return or escalation.
- Trigger direct detection and verify alert/chase behavior.

### Do-Not-Touch Notes

- Do not depend on undocumented ALS sample `BT_Als` or `BB_Als` semantics unless they are explicitly inspected and documented.
- Do not move guard brain ownership into ALS sample AI.

### Expected Output

- A legible guard loop that opens and closes the stealth route based on player-created stimuli.

## Task 8: Demo Level Blockout

### Goal

Create the compact stealth lane used to prove the whole loop: spawn, patrol space, lit route, shadow route, lure/noise point, objective, extraction, and debug visibility.

### Allowed Files / Scope

- `/Game/Stealth/Maps/**`
- `/Game/Stealth/Blueprints/**`
- `/Game/Stealth/Data/**`
- `Docs/demo/**` for level notes

### Dependencies

- TH-511 readiness, or repeat its required smoke test.
- Tasks 3-7 can be stubbed initially, but the map must be prepared to host their actors.

### Acceptance Criteria

- Prototype map exists under `/Game/Stealth/Maps/`, preferably `/Game/Stealth/Maps/L_StealthAls_Prototype`.
- Map contains player start, one guard, patrol route, lit route, shadow route, lure/noise point, objective placeholder, extraction placeholder, and debug surface.
- Level layout supports the gameplay beat sheet from the gameplay design doc.
- Assets are clearly dev/prototype scoped.

### Verification

- Start PIE in the prototype map.
- Confirm the player can traverse both lit and shadow routes.
- Verify the guard, route, lure, objective placeholder, extraction placeholder, and debug surface are present through outliner/readback or visible play.

### Do-Not-Touch Notes

- Do not alter root production maps unless explicitly requested.
- Do not place prototype content in ALS plugin content.

### Expected Output

- A small playable testbed for the stealth ALS loop.

## Task 9: Objective And Extraction Loop

### Goal

Complete the mission loop: interact with objective, unlock extraction, and report success or compromised completion.

### Allowed Files / Scope

- `Source/echelon_05/Stealth/**`
- `/Game/Stealth/Blueprints/**`
- `/Game/Stealth/UI/**`
- `/Game/Stealth/Maps/**`

### Dependencies

- Task 3.
- Task 8.
- Task 6 or Task 7 for compromised/alert state, though objective/extraction can be built against stubs first.

### Acceptance Criteria

- Objective starts incomplete and can be completed by player interaction.
- Extraction is unavailable before objective completion.
- Extraction becomes available after objective completion.
- Entering/using extraction after objective completion records mission success.
- If alert/compromised state exists, final state can show clean success versus compromised success/failure.
- Objective and extraction states appear in debug output.

### Verification

- Try extraction before objective and verify it does not complete the mission.
- Complete objective, return to extraction, and verify mission success.
- Confirm debug/UI state changes at each step.

### Do-Not-Touch Notes

- Do not add inventory, scoring, save/load, or combat depth for this first loop.

### Expected Output

- A complete playable beginning-middle-end loop for the prototype.

## Task 10: Playtest Checklist, Tuning Pass, And Handoff

### Goal

Run the end-to-end scenarios, tune relationships for readability, and document what is ready versus deferred.

### Allowed Files / Scope

- `Docs/demo/**`
- `Docs/Implementation/**`
- tuning data under `/Game/Stealth/Data/**`
- small fixes in `Source/echelon_05/Stealth/**` or `/Game/Stealth/**` only when needed to satisfy acceptance

### Dependencies

- Tasks 1-9.

### Acceptance Criteria

- Safe sneak, intentional detection, lure, and extraction gate scenarios are all run and documented.
- Tuning preserves the intended relationships:
  - crouched in shadow is safest
  - sprinting in light is dangerous
  - lure sound attracts investigation without always forcing instant alert
  - objective gates extraction
- Remaining gaps and deferrals are listed.
- Any known flaky behavior or UE-MCP workaround is documented.

### Verification

- Run the prototype map in PIE.
- Record pass/fail notes for each test scenario from `Docs/demo/STEALTH_ALS_PROTOTYPE_IMPL.md`.
- Confirm no ALS plugin source/content/config files were modified.

### Do-Not-Touch Notes

- Do not convert tuning into final balance work.
- Do not expand into multiplayer, squad tactics, advanced acoustic propagation, or production art.

### Expected Output

- A concise playtest/tuning handoff showing that the prototype proves the intended ALS -> ECS -> guard -> mission loop.

## Execution Order Inside TH-513

These tasks are an implementation guide for the single Linear issue TH-513. Do not create separate Linear subtasks or child issues unless explicitly requested.

The recommended first implementation wave is:

1. Task 1: Foundation types.
2. Task 2: ALS snapshot adapter.
3. Task 3: Debug overlay foundation.

After those land, Tasks 4 and 5 can proceed in parallel if the implementation agent has enough context. Tasks 6 and 7 should follow emissions. Task 8 can proceed in parallel as a blockout if it stays scoped to dev content. Tasks 9 and 10 close the playable loop.
