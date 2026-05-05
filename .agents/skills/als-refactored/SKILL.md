---
name: als-refactored
description: Use when working with ALS Refactored to understand its codebase, inspect ALS locomotion behavior, read /ALS package paths, navigate Plugins/ALS-Refactored, or plan project-side integrations.
---

# ALS Refactored

## Purpose

Use this skill when working with ALS Refactored in `echelon_05` to understand the plugin codebase, asset structure, Blueprint behavior, and C++/content boundaries.

The project is built around `Plugins/ALS-Refactored` as an upstream plugin dependency. The plugin should stay frozen to preserve its upgrade path and avoid local drift.

## Non-Negotiable Rule

Do not edit files under `Plugins/ALS-Refactored/` unless the user explicitly authorizes plugin changes.

Default to project-side work:

- Host game C++ modules under `Source/`
- Root/project `Config/`
- Root/project `Content/`
- Project-specific wrapper, subclass, integration, or adapter layers

If a feature seems to require plugin edits, stop and explain the tradeoff:

- Keep ALS frozen and build externally
- Fork/modify ALS and accept plugin drift

## Read-First Workflow

Before answering or implementing ALS work, read the relevant docs:

- `AGENTS.md`
- `Plugins/ALS-Refactored/AGENTS.md`
- `Plugins/ALS-Refactored/SYSTEM_MAP.md`
- `Plugins/ALS-Refactored/Content/AGENTS.md`

For packaged asset behavior, also read:

- `Plugins/ALS-Refactored/Content/ALS/AGENTS.md`
- `Plugins/ALS-Refactored/Content/ALS/Character/AGENTS.md`
- `Plugins/ALS-Refactored/Content/ALSExtras/AGENTS.md`
- `Plugins/ALS-Refactored/Content/ALSCamera/AGENTS.md`

## UE-MCP Rules

When using UE-MCP:

1. Load `ue-mcp-workflow`.
2. For Blueprint work, also load `ue-mcp-blueprint`.
3. Read the MCP tool descriptor JSON before calling any MCP tool.
4. Call `project(action="get_status")` first.
5. Use Unreal package paths, not filesystem paths.

Package path mapping:

- Filesystem: `Plugins/ALS-Refactored/Content/...`
- Unreal mount: `/ALS/...`
- Example: `Plugins/ALS-Refactored/Content/ALS/Character/B_Als_Character.uasset` maps to `/ALS/ALS/Character/B_Als_Character`.

Known caveat: native `asset(action="list")` may not enumerate plugin mounts, but `blueprint(...)` can read known `/ALS/...` package paths.

## Blueprint Reading Workflow

Use this order:

1. `blueprint(action="read", assetPath="/ALS/...")`
2. `blueprint(action="list_graphs", assetPath="/ALS/...")`
3. `blueprint(action="read_graph_summary", assetPath="/ALS/...", graphName="...")`
4. Use full `read_graph` only for focused graph inspection.

Do not document behavior from filenames alone. Use graph evidence, C++ evidence, or mark the detail as a gap.

## Behavior Anchors

- C++ owns core ALS locomotion state refresh, movement, mantling, ragdolling, rolling, input callbacks, and camera handoff.
- `B_Als_Character` handles overlay presentation: overlay object refresh, linked overlay animation layers, static/skeletal overlay attachment, and overlay clearing during mantle/ragdoll.
- `AB_Als` is the main animation instance. Linked AnimBPs split locomotion, grounded state, standing/crouching stances, layering, ragdoll, head/view, and overlay output.
- `B_Als_PlayerController` owns optional sample UI: HUD/menu creation, ALSExtras input mapping, overlay menu navigation, UI toggle, and slomo.
- ALSExtras moving/rotating environment actors derive motion from server time plus ping compensation.
- AI task Blueprints are readable. `BT_Als` and `BB_Als` behavior/key semantics remain a gap unless inspected with an appropriate tool.

## Documentation Rule

Documentation under `Plugins/ALS-Refactored` may be updated to capture evidence about the frozen plugin.

Documentation is not permission to alter plugin runtime source, assets, or config. Keep doc edits factual, evidence-based, and clear about gaps.
