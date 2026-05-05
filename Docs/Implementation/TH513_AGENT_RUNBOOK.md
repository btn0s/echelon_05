# TH-513 Agent Runbook

## Purpose

This runbook is the small-model / junior-agent entry point for Linear TH-513: implement the stealth ALS prototype demo from the task breakdown doc.

Use this file to start the task, make the default architecture choices, verify work, and know when to stop.

## Start Here

1. Read `README.md`.
2. Read `AGENTS.md`.
3. Read `Docs/Implementation/STEALTH_ALS_PROTOTYPE_TASKS.md`.
4. Read `Docs/demo/STEALTH_ALS_PROTOTYPE_IMPL.md`.
5. Read `Docs/Implementation/UE_MCP_READINESS.md` before any Blueprint or level authoring.
6. Do not create Linear subtasks or child issues unless explicitly requested.
7. Work inside the single Linear issue TH-513.

## Fixed Decisions For First Pass

Use these defaults unless a human explicitly changes the plan:

- Native C++ owns reusable stealth simulation state and core logic.
- Blueprints may own prototype actor setup, map placement, tuning data, simple UI, and debug presentation.
- The guard may start as a simple project-owned non-ALS actor.
- ALS-backed guard presentation is optional polish, not required for the first pass.
- Footsteps use fallback distance/time polling first.
- Do not build an animation-notify footstep bridge until the fallback loop is working.
- Do not build a general-purpose ECS framework before the demo loop works.
- Keep prototype content isolated under `/Game/_Dev/StealthDemo`.
- Keep C++ implementation under `Source/echelon_05/Stealth`.

## Allowed Scope

Allowed implementation areas:

- `Source/echelon_05/`
- project `Content/`, preferably `/Game/_Dev/StealthDemo`
- project `Config/` only when a project-owned setting is required
- `Docs/demo/`
- `Docs/Implementation/`

Do not edit:

- `Plugins/ALS-Refactored/Source/`
- `Plugins/ALS-Refactored/Content/`
- `Plugins/ALS-Refactored/Config/`

If the task appears to require ALS plugin edits, stop and report the tradeoff.

## Recommended Work Order

Follow the execution phases in `Docs/Implementation/STEALTH_ALS_PROTOTYPE_TASKS.md`:

1. Foundation stealth types and runtime container.
2. ALS snapshot adapter.
3. Debug overlay foundation.
4. Light and visibility emissions.
5. Sound events and footstep fallback.
6. One guard perception and suspicion.
7. Patrol, investigate, chase, and return intent.
8. Demo level blockout.
9. Objective and extraction loop.
10. Playtest checklist, tuning pass, and handoff.

Do not create separate Linear issues for these phases.

## Build And Launch Commands

From the repo root:

```powershell
npm run build
```

To launch the editor:

```powershell
npm run launch
```

For a clean rebuild:

```powershell
npm run rebuild
```

If a command fails because local Unreal paths or environment assumptions are missing, stop and report the exact command and error.

## UE-MCP Rules

Before Blueprint or level work, confirm the editor bridge:

1. Call `project(action="get_status")`.
2. Confirm the editor is connected to `echelon_05`.
3. Use Unreal package paths, not filesystem paths.
4. Read existing assets before mutating.
5. Compile after Blueprint mutations.
6. Save and verify placement/readback after level mutations.

Known prototype map target:

```text
/Game/_Dev/StealthDemo/Maps/L_StealthAls_Prototype
```

Known ALS Blueprint package path:

```text
/ALS/ALS/Character/B_Als_Character
```

## Required PIE Scenarios

Before handing off, run or attempt these scenarios in PIE:

### Safe Sneak

- Crouch-walk through the shadow route.
- Guard should not reach alert.
- Objective and extraction should still work.

### Intentional Detection

- Stand or sprint through lit line-of-sight.
- Visibility/noise should rise.
- Guard suspicion should reach alert or equivalent pressure state.

### Lure

- Trigger the lure/noise source.
- Guard should receive the sound stimulus and investigate or look toward it.

### Extraction Gate

- Try extraction before objective.
- Complete objective.
- Try extraction again.
- Extraction should only complete the mission after objective completion.

## Stop Conditions

Stop and report instead of guessing if:

- UE-MCP cannot connect to the editor.
- The project does not build before gameplay work begins.
- A required implementation path appears to need edits under `Plugins/ALS-Refactored`.
- You need to alter ALS animation assets or ALS notifies.
- You cannot identify whether a binary asset belongs with an intentional map change.
- The prototype requires a broad architecture change not described in the docs.
- You are about to create new Linear subtasks or child issues.

## Handoff Note Template

Leave this note on TH-513 and/or in `Docs/Implementation/` when finished or blocked:

```md
## TH-513 Handoff

Implemented:

Map path:

Build result:

PIE scenarios:
- Safe sneak:
- Intentional detection:
- Lure:
- Extraction gate:

Files changed:

ALS plugin touched:

Known gaps / blockers:

Next recommended action:
```

