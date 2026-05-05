# TH-517 Handoff — Scene-light body sampling

## Summary

Replaced stealth **volume-authored** light exposure as the normal gameplay path with **scene-light body sampling** (head / chest / feet). Guards still consume `FVisibilityEmitter` (`LightExposure`, `CurrentVisibility`) unchanged.

## Implementation files

- [`Source/echelon_05/Stealth/Types/StealthTypes.h`](../../Source/echelon_05/Stealth/Types/StealthTypes.h) — `FStealthLightContributionDebug`, `FStealthBodyLightSampleDebug`, `FStealthLightSamplingDebug`
- [`Source/echelon_05/Stealth/Data/StealthTuningDataAsset.h`](../../Source/echelon_05/Stealth/Data/StealthTuningDataAsset.h) — `LightSampling` category (offsets, cache interval, occlusion channel, smoothing, normalization, etc.)
- [`Source/echelon_05/Stealth/Subsystems/StealthSimulationSubsystem.h`](../../Source/echelon_05/Stealth/Subsystems/StealthSimulationSubsystem.h) / [`.cpp`](../../Source/echelon_05/Stealth/Subsystems/StealthSimulationSubsystem.cpp) — cached light scan, `ComputeSceneLightExposureAt`, `SampleLightExposureAt` (scene-only), `SampleBodyLightExposureMaxBias`, `GetLastLightSamplingDebug`
- [`Source/echelon_05/Stealth/Components/StealthEmitterComponent.cpp`](../../Source/echelon_05/Stealth/Components/StealthEmitterComponent.cpp) — builds body points from tuning offsets; calls `SampleBodyLightExposureMaxBias`; debug draw when `stealth.DebugDraw > 0`
- [`Source/echelon_05/Stealth/UI/StealthDebugHUD.cpp`](../../Source/echelon_05/Stealth/UI/StealthDebugHUD.cpp) — prints scene-light summary + top contributors per sample

Legacy **`AStealthLightVolume`** remains in code but is **no longer used** by `SampleLightExposureAt` (subsystem ignores volumes for sampling).

## Tuning / debug

| Control | Purpose |
|--------|---------|
| `LightSampling` on [`UStealthTuningDataAsset`](../../Source/echelon_05/Stealth/Data/StealthTuningDataAsset.h) | Head/chest/feet offsets, cache refresh, max distances, occlusion channel, ambient baseline, local-light intensity normalization, directional-light reference intensity, max-bias blend, smoothing half-life |
| Console **`stealth.DebugDraw`** | `> 0`: guard cone/hearing (existing) **and** scene-light sample spheres, rays to contributing lights, labels |

## Build

- `npm run build` — **Succeeded** (Editor Win64 Development).

## Known mismatch cases (non-goals for this pass)

- No GPU readback; exposure does **not** match **Lumen / skylight / exposure / post-process / indirect bake / emissive** faithfully.
- **SkyLight** components are skipped (fills handled loosely via `SceneLightAmbientExposure`).
- **Intensity units** are simplified into heuristics: local lights use `SceneLightIntensityNormalization`, while directional lights use `SceneLightDirectionalReferenceIntensity`.
- Maps still placing **`AStealthLightVolume`** will **not** change stealth exposure until volumes are removed or lighting alone carries contrast — tune scene lights + normalization accordingly.

## Quick PIE check

1. Open `/Game/Stealth/Maps/L_StealthAls_Prototype` (or your stealth demo map).
2. Verify stealth HUD shows `SceneLight:` lines and per-sample exposures update when standing under movable/static lights.
3. Run **`stealth.DebugDraw 1`** — cyan spheres at samples; green/red segments toward evaluated lights.
