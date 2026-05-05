# echelon_05

Unreal Engine 5.7 project workspace for the Echelon prototype.

The current planning focus is the stealth ALS prototype demo: a small playable loop that connects ALS Refactored locomotion facts to project-side stealth simulation, guard perception, objective completion, extraction, and debug explanation.

## Repository Shape

```text
Config/                         Project configuration
Content/                        Project content and maps
Docs/demo/                      Gameplay and implementation design docs
Docs/Implementation/            Agent task and UE-MCP readiness docs
Plugins/ALS-Refactored/         Frozen upstream ALS Refactored plugin
Plugins/BlueprintAutoLayout/    Editor-only Blueprint graph layout helper
Source/echelon_05/              Project C++ module
scripts/                        Node helpers for build, launch, and cleanup
```

## Read First

- `AGENTS.md` for workspace rules and ALS scope.
- `Docs/demo/STEALTH_DEV_DEMO_GAMEPLAY.md` for the gameplay target.
- `Docs/demo/STEALTH_ALS_PROTOTYPE_IMPL.md` for the implementation guide.
- `Docs/Implementation/TH513_AGENT_RUNBOOK.md` for the junior-agent handoff/runbook.
- `Docs/Implementation/STEALTH_ALS_PROTOTYPE_TASKS.md` for the TH-513 execution guide.
- `Docs/Implementation/UE_MCP_READINESS.md` for UE-MCP Blueprint authoring notes.
- `Plugins/ALS-Refactored/SYSTEM_MAP.md` for validated ALS plugin anchors.

## ALS Refactored Scope

`Plugins/ALS-Refactored` is treated as a frozen upstream plugin dependency. Do not edit ALS runtime source, plugin config, or plugin content unless a task explicitly authorizes a fork/patch path.

Project-side integration should live in:

- `Source/echelon_05/`
- root/project `Content/`
- root/project `Config/` only when a project-owned setting is required
- docs under `Docs/`

## Stealth ALS Prototype

Linear TH-513 is the single implementation issue for the prototype. It points to the 10-task repo guide rather than creating a Linear subtask tree.

Target loop:

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

Prototype content should be isolated under:

```text
/Game/_Dev/StealthDemo/
```

Recommended map path:

```text
/Game/_Dev/StealthDemo/Maps/L_StealthAls_Prototype
```

## Git LFS

Git LFS is required. Unreal binary assets are tracked through `.gitattributes`, including:

- `*.uasset`
- `*.umap`
- common source art, image, and audio formats

After cloning, run:

```powershell
git lfs install
git lfs pull
```

World Partition / One File Per Actor payloads under `Content/__ExternalActors__/...` are intentional Unreal assets. Commit them with their matching intentional `.umap` changes.

## Build And Launch

The Node scripts wrap the local Unreal Engine 5.7 workflow:

```powershell
npm install
npm run build
npm run launch
```

Useful scripts:

```powershell
npm run build          # Build editor target
npm run build-game     # Build game target
npm run dev            # Build then launch
npm run clean          # Remove generated build folders
npm run clean-logs     # Clean logs
npm run rebuild        # Clean then build
```

## UE-MCP Notes

UE-MCP is enabled for editor automation. Before Blueprint or level authoring, follow the readiness discipline documented in `Docs/Implementation/UE_MCP_READINESS.md`:

- Start with `project(action="get_status")`.
- Use Unreal package paths, not filesystem paths.
- Read before mutating.
- Compile after Blueprint mutations.
- Save and verify placement/readback after level mutations.

ALS plugin content is mounted under `/ALS`. For example:

```text
/ALS/ALS/Character/B_Als_Character
```

## BlueprintAutoLayout

`Plugins/BlueprintAutoLayout` is an editor-only helper plugin that adds an **Organize Selected Nodes** command to Blueprint graph context menus. It is useful for keeping prototype Blueprint graphs readable, but it is not the main purpose of this repository.

## Generated Files

The repo ignores Unreal-generated and local-only folders such as:

- `Binaries/`
- `Intermediate/`
- `Saved/`
- `DerivedDataCache/`
- `.cursor/`
- IDE folders such as `.vs/`

Do not commit generated build products unless a task explicitly requires a checked-in artifact.
