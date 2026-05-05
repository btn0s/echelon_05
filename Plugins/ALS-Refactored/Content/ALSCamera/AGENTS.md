# AGENTS.md — Plugins/ALS-Refactored/Content/ALSCamera/

Camera-facing ALS assets. Files are binary `*.uasset`; use UE-MCP package paths under `/ALS/ALSCamera/...`.

## Blueprint Inventory

| Asset | UE-MCP notes |
|------|--------------|
| `B_Als_CameraComponent.uasset` | Blueprint class `B_Als_CameraComponent`, parent `AlsCameraComponent`; no non-empty graphs observed. |
| `AB_Als_Camera.uasset` | Animation Blueprint class `AB_Als_Camera`, parent `AlsCameraAnimationInstance`; contains `AnimGraph`, `Look States`, and `Velocity Direction` / `View Direction` / `Aiming` state graphs. |
| `Data/B_Als_CameraShake_Sprint.uasset` | Blueprint class `B_Als_CameraShake_Sprint`, parent `CameraShakeBase`; no non-empty graphs observed. |

## Behavior Notes

Camera runtime behavior is mostly C++ in `UAlsCameraComponent` / `UAlsCameraAnimationInstance`; the assets here provide authored data and animation layering.

- `B_Als_CameraComponent` is a Blueprint subclass of `AlsCameraComponent` with no non-empty graphs observed. Treat it as a defaults/configuration wrapper unless focused property reads show otherwise.
- `AB_Als_Camera` is the camera skeletal animation instance. Its `Look States` state machine branches into `Velocity Direction`, `View Direction`, and `Aiming` state graphs, matching the ALS camera/view-mode concepts from C++.
- `B_Als_CameraShake_Sprint` is a `CameraShakeBase` asset with no graph logic observed; shake behavior is likely in defaults/pattern data rather than Blueprint nodes.

Behavior gap: exact camera offsets, curve values, blend behavior, and shake parameters are not established by graph names. Read properties or inspect in Editor before documenting numeric behavior.

## Data Assets

`Data/CF_Als_CameraBlend_Smooth.uasset` and related curves are camera blend data. Use `asset(action="read_properties")` or Editor inspection for curve/default details.

## Agent Rules

1. Prefer C++ evidence in `Source/ALSCamera/` for camera behavior, then use UE-MCP reads for Blueprint/AnimBP asset wiring.
2. Do not infer camera curve values or shake parameters from filenames; read properties or inspect in Editor.
