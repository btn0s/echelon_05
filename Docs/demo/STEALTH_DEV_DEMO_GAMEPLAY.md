# Stealth ALS Dev Demo - Gameplay Design

## Purpose

This document defines the gameplay target for the first stealth dev demo level. It is intentionally gameplay-first: it describes what the player should experience and what the demo must prove, without depending on Unreal class names, Blueprint graphs, or ECS implementation details.

Implementation details belong in [STEALTH_ALS_PROTOTYPE_IMPL.md](STEALTH_ALS_PROTOTYPE_IMPL.md).

## Source Context

- Linear TH-509: gameplay design doc request.
- Linear TH-507: merged ALS locomotion layer plus stealth ECS simulation architecture.
- Linear TH-506: minimal stealth ECS loop.
- ALS local docs under `Plugins/ALS-Refactored`, especially `SYSTEM_MAP.md`.

## Demo Fantasy

The player is an infiltrator crossing a compact guarded space to steal a small objective and extract. The demo should feel like a readable stealth toy box: every important outcome should make sense from the player's posture, speed, light exposure, noise, and guard attention.

The core promise is simple:

> Move carefully through shadow, use sound to redirect the guard, take the objective, and extract. Move loudly or visibly and the guard escalates.

## Player Experience Target

The level should teach through play, not text. Within one minute, the player should understand:

- Crouching helps.
- Shadow helps.
- Sprinting is risky.
- Sound pulls attention.
- A guard can be curious before becoming fully alerted.
- The objective and extraction create a complete mission loop.
- The debug overlay explains why the simulation made a decision.

This is a developer demo, so clarity beats realism. The player should be able to intentionally create both success and failure in a repeatable space.

## Player Verbs

| Verb | Gameplay meaning | Demo proof |
|------|------------------|------------|
| Sneak | Move slowly/crouched through safer space. | Player can pass a guard with low visibility/noise. |
| Sprint | Move quickly through danger. | Player rapidly creates visibility/noise and can trigger detection. |
| Observe | Read guard state, patrol path, light/shadow, and debug feedback. | Player can understand why suspicion changes. |
| Lure | Create a sound away from the intended route. | Guard investigates the sound location, opening a path. |
| Interact | Use the objective or a simple world object. | Interaction changes objective or emits noise. |
| Extract | Leave after objective completion. | Mission completes only after objective plus extraction. |

## Demo Level Concept

Use one small level shaped like a readable stealth lane:

1. **Start alcove:** Player spawns outside immediate guard view.
2. **Patrol space:** One guard patrols across the middle of the level.
3. **Lit route:** Short, exposed path across the guard's view. Fast but dangerous.
4. **Shadow route:** Slightly longer, darker path with cover or reduced exposure.
5. **Noise/lure point:** A throwable object, noisy door, button, or simple sound source positioned to pull the guard away.
6. **Objective:** A terminal, case, or intel pickup behind or near the guard's route.
7. **Extraction:** Exit trigger near the start or opposite side, unlocked after the objective.

The level should be compact enough that designers and agents can test the whole loop quickly.

## Beat Sheet

### Beat 1: Spawn and Read

The player starts safe. They can see the objective area, the guard patrol route, and at least one obvious light/shadow contrast.

Expected result:

- Guard begins unaware.
- Objective is incomplete.
- Extraction is unavailable.
- Debug overlay shows baseline visibility/noise.

### Beat 2: Safe Sneak Route

The player crouch-walks or moves carefully through the shadow route.

Expected result:

- Visibility stays low.
- Footstep noise stays low.
- Guard suspicion remains low or only briefly rises.
- Player can reach the objective without full alert.

### Beat 3: Failure Route

The player sprints or stands in the lit route while the guard has line of sight.

Expected result:

- Visibility and noise rise clearly.
- Guard suspicion climbs quickly.
- Sustained exposure escalates to alert/chase.
- Debug overlay makes the cause obvious.

### Beat 4: Sound Lure

The player triggers a noise away from the stealth route or objective.

Expected result:

- A sound stimulus appears.
- Guard turns toward or moves to the sound.
- Guard state becomes curious/investigating instead of instantly alert unless the sound is extreme.
- Player can exploit the opening.

### Beat 5: Objective

The player interacts with the objective.

Expected result:

- Objective state changes to complete.
- Extraction becomes available.
- Objective interaction may emit a small sound if useful for the demo.

### Beat 6: Extract

The player reaches extraction after completing the objective.

Expected result:

- Mission completes.
- Debug overlay or simple UI reports success.
- If the player reaches extraction before completing the objective, extraction remains unavailable.

## Guard Behavior Expectations

The first guard should be simple and legible.

| State | Player-facing behavior |
|-------|------------------------|
| Unaware | Follows patrol and ignores low-level movement/noise. |
| Curious | Pauses or looks toward weak sound/glimpse. |
| Suspicious | Moves cautiously toward last stimulus or last known position. |
| Investigating | Searches a small area near the stimulus. |
| Alert | Commits to chase, call alarm, or fail-state behavior. |
| Return | Goes back to patrol after suspicion decays. |

The guard should not need sophisticated tactics. The first demo is about causality: player movement changes stealth emissions, stealth emissions change guard suspicion, guard suspicion changes behavior.

## Success States

The main success state:

- Objective complete.
- Extraction used.
- Player was not fully detected, or detection is allowed but not required to fail the mission depending on tuning.

Optional developer success checks:

- Complete the loop with no alert.
- Complete the loop after using a lure.
- Trigger alert intentionally, then reset or replay quickly.

## Failure And Pressure States

The demo does not need a harsh fail screen, but it needs visible failure pressure.

Acceptable first-pass failure states:

- Guard reaches alert and chases the player.
- Alarm level rises.
- Extraction remains unavailable without objective.
- Debug overlay marks the run as compromised.

Avoid early complexity:

- No full combat depth.
- No save/load.
- No scoring screen.
- No multi-guard coordination.

## Readability Rules

The player should never wonder why the system reacted.

When the player is seen, the demo should make these causes inspectable:

- stance: standing versus crouching
- movement: idle/walk/run/sprint
- light exposure: shadow versus lit
- line of sight: inside or outside guard cone
- suspicion value: current and threshold

When the player is heard, the demo should make these causes inspectable:

- movement speed/gait
- recent footstep or sound event
- noise radius
- guard distance from sound
- guard hearing response/state

Debug visibility is part of the design, not just a developer convenience. The first demo lives or dies on whether causality is obvious.

## Tuning Intent

Use exaggerated, readable tuning at first.

| Situation | Intended feel |
|-----------|---------------|
| Crouched in shadow | Safe unless very close or directly observed for long. |
| Standing in shadow | Risky but not instant failure. |
| Walking in light | Noticeable; suspicion rises if guard is looking. |
| Sprinting in light | Dangerous; quick suspicion rise and loud noise. |
| Sound lure | Pulls curiosity/investigation, not automatic alert. |
| Objective interaction | Clear state change; optional minor sound. |

Numbers can be rough. Relationships matter more than realism.

## Level Ingredient Checklist

- Player spawn.
- One ALS-controlled player character.
- One guard.
- One patrol route with 2-4 points.
- One lit danger route.
- One shadow/safer route.
- One sound/lure source.
- One objective interactable.
- One extraction zone.
- One debug overlay.
- Reset/replay path for rapid iteration.

## Non-Goals

- Rewriting ALS Refactored.
- Forking ALS animation graphs for stealth-specific logic.
- Treating prone as built-in ALS support.
- Complex guard squad tactics.
- Utility AI.
- Advanced acoustic propagation.
- Photorealistic light probes.
- Large inventory.
- Combat depth.
- Multiplayer replication.
- Final art, cinematic presentation, or production mission layout.

## Gameplay Acceptance

The gameplay design is ready when:

- A human can describe the full demo loop after reading this doc.
- The demo can be tested in under a few minutes.
- Each player verb has a concrete level beat that proves it.
- Success, failure, suspicion, and extraction are all represented.
- The implementation guide can map each beat to a system or actor without inventing new gameplay goals.
