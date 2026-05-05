## TH-513 Handoff

Implemented:

- Project-side stealth C++ skeleton exists under `Source/echelon_05/Stealth`.
- Runtime state/subsystem, ALS adapter, debug HUD, visibility and sound emitters, light volumes, lure sound actor, guard perception/suspicion/brain, patrol route, objective, extraction zone, interactor component, tuning data asset class, and demo game mode are present.
- Follow-up fixes made in this pass:
  - Added direct `CoreMinimal.h` includes to stealth enum/type headers.
  - Fixed ALS adapter component tick prerequisite wiring.
  - Fixed light volume point containment to test local-space box extents.
  - Aligned overlapping light-volume sampling with the plan's highest-exposure-wins rule.
  - Changed interaction probing from a line trace to the planned sphere sweep.
  - Updated guard hearing so continuous movement noise can update last-known player position.
  - Added project-side Enhanced Input binding support to `UStealthInteractorComponent`.
  - Created `/Game/_Dev/StealthDemo` folders, Blueprint wrappers, tuning data, use input action/mapping context, and the prototype level.

Map path:

- `/Game/_Dev/StealthDemo/Maps/L_StealthAls_Prototype`.
- The map contains a player start, stealth demo GameMode override, guard, patrol route, lit/shadow volumes, lure source, objective, extraction zone, blockout floor/cover, directional light, and navmesh bounds.

Build result:

- `npm run build` succeeded after the editor was closed.
- `npm run dev` succeeded through build and launched the editor.
- Later interactor input-binding source changes were applied with Live Coding successfully.
- Because those later changes add reflected properties, perform one final editor-closed `npm run build` before treating the branch as fully restart-verified.
- npm also warned that npm `11.7.0` does not support the current Node `20.13.1`, but this was not the build-stopping error.

PIE scenarios:

- Safe sneak: Partial smoke only. PIE starts, spawns `BP_StealthDemo_Player`, and runtime actors are present; manual stealth route tuning still needs playthrough.
- Intentional detection: Partial smoke only. Guard debug function responds and reported sight/suspicion during PIE; full alert/chase scenario still needs manual playthrough.
- Lure: Partial smoke passed. `Stealth_LureNoiseSource.TriggerLure()` invoked successfully in PIE; guard behavior still needs tuned/manual observation.
- Extraction gate: Partial smoke passed. Objective interaction invoked in PIE and logged `Objective completed by BP_StealthDemo_Player_C_0`; pre/post extraction gate still needs a cleaner native-tool verification.

Files changed:

- `Source/echelon_05/echelon_05.Build.cs`
- `Source/echelon_05/Stealth/Actors/StealthGuard.cpp`
- `Source/echelon_05/Stealth/Actors/StealthLightVolume.cpp`
- `Source/echelon_05/Stealth/Actors/StealthObjective.cpp`
- `Source/echelon_05/Stealth/Actors/StealthObjective.h`
- `Source/echelon_05/Stealth/Components/StealthAlsAdapterComponent.cpp`
- `Source/echelon_05/Stealth/Components/StealthInteractorComponent.cpp`
- `Source/echelon_05/Stealth/Components/StealthInteractorComponent.h`
- `Source/echelon_05/Stealth/Subsystems/StealthSimulationSubsystem.cpp`
- `Source/echelon_05/Stealth/Types/StealthEnums.h`
- `Source/echelon_05/Stealth/Types/StealthTypes.h`
- `Content/_Dev/StealthDemo/**`
- `Docs/Implementation/TH513_HANDOFF.md`

ALS plugin touched:

- None. `git status --short -- Plugins/ALS-Refactored` returned no changes.

Known gaps / blockers:

- Need one final editor-closed `npm run build` after the reflected interactor input properties were added.
- PIE private subsystem state could not be fully inspected through native UE-MCP tools; several attempts used `editor(action="execute_python")` as a workaround.
- The level is a functional blockout, not tuned final gameplay. Run manual route checks and tune `DA_StealthTuning`.
- The extraction gate needs a cleaner native-tool test for pre-objective blocked extraction and post-objective success.

Next recommended action:

1. Close the editor and rerun `npm run build`.
2. Reopen the map, manually run the four scenarios, and tune `DA_StealthTuning`.
3. If UE-MCP feedback is desired, submit a native-tool gap for PIE subsystem getter inspection / PIE actor movement by label.
