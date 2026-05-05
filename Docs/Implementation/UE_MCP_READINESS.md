# UE-MCP Blueprint authoring readiness (TH-511)

This spike documents whether the Unreal Editor bridge (UE-MCP) is ready for agent-driven Blueprint work on **echelon_05** before building prototype gameplay Blueprints.

**Linear:** [TH-511 — Prove UE-MCP Blueprint authoring readiness before gameplay](https://linear.app/thinkhumanco/issue/TH-511/prove-ue-mcp-blueprint-authoring-readiness-before-gameplay)

**Run date:** 2026-05-04 (smoke test executed against live editor)

---

## Disposable dev assets (project content)

| Asset / path | Purpose |
| --- | --- |
| `/Game/_Dev/MCP/` | Dev folder for MCP smoke artifacts (`asset(action="create_folder")`). |
| `/Game/_Dev/MCP/BP_MCP_SmokeTest` | Disposable `Actor` Blueprint: extra `SceneComponent` (`McpSmokeRoot`), `int` variable `SmokeTestCounter`, **EventGraph** wired **Event BeginPlay → KismetSystemLibrary::PrintString** (`InString` = `[MCP SmokeTest] BeginPlay fired`), compiled successfully. |
| `L_Als_Playground` | Existing playground map used to place one instance (`MCP_SmokeTest_00` at origin + Z=500), then saved. |

Remove or ignore these when cleaning dev state; they are not part of shipping gameplay.

**Verifying behavior:** PIE with `L_Als_Playground` loaded should show the on-screen print (and log line) when `MCP_SmokeTest_00` runs BeginPlay. The original TH-511 pass only exercised SCS/variables; graph `add_node` / `connect_pins` / `set_node_property` was added afterward to confirm node authoring works end-to-end.

---

## Smoke test checklist (required)

| Step | Result | Notes |
| --- | --- | --- |
| 1. `project(action="get_status")` | **Pass** | `mode: live`, `editorConnected: true`, project **echelon_05**, engine **5.7**, `.uproject` path reported. |
| 2. Read ALS Blueprint `/ALS/ALS/Character/B_Als_Character` | **Pass** | Parent `AlsCharacterExample`; SCS lists `OverlaySkeletalMesh`, `OverlayStaticMesh`. |
| 3. List graphs + lightweight graph summary | **Pass** | `list_graphs` returned 7 graphs (incl. `EventGraph`, 15 nodes). `read_graph_summary` on `EventGraph` returned nodes + exec/data edges. |
| 4. Create disposable Blueprint `/Game/_Dev/MCP/BP_MCP_SmokeTest` parent `Actor` | **Pass** | See workarounds below — initial attempts failed until folder + path shape were corrected. |
| 5. Add component + variable | **Pass** | `McpSmokeRoot` (`SceneComponent` under default root); variable `SmokeTestCounter` (`int`). |
| 6. Compile + read back | **Pass** | `compile` succeeded; `read` shows both components; `list_variables` shows `SmokeTestCounter`. |
| 7. Place in level | **Pass** | `place_actor` with `actorClass` `/Game/_Dev/MCP/BP_MCP_SmokeTest.BP_MCP_SmokeTest_C`, label `MCP_SmokeTest_00`. |
| 8. Save + verify outliner | **Pass** | `level(action="save")`; `get_outliner` with `nameFilter: MCP_SmokeTest` returns the placed actor and expected components. |
| 9. Document gaps / workarounds | **Pass** | This section below. |

---

## Tool gaps, errors, and workarounds

### 1. Content folder must exist before Blueprint create

`blueprint(action="create", …)` returned **"Failed to create Blueprint"** when `/Game/_Dev/MCP` did not exist.

**Workaround:** call `asset(action="create_folder", path="/Game/_Dev/MCP")` (or `paths[]` for several folders) first.

### 2. `blueprint` `create` `assetPath` shape

Using a full object-style path **failed**:

- Failed: `/Game/_Dev/MCP/BP_MCP_SmokeTest.BP_MCP_SmokeTest`

Using a **package-style** path (no `.AssetName` suffix) **succeeded**:

- OK: `/Game/_Dev/MCP/BP_MCP_SmokeTest`

The handler response then exposes `objectPath` `/Game/_Dev/MCP/BP_MCP_SmokeTest.BP_MCP_SmokeTest` for subsequent `read` / `compile` / etc.

**Guidance for agents:** prefer package path for `create`; use `objectPath` from the response (or the usual `/Path/Name.Name` form) for reads and mutations.

### 3. Spawning Blueprint-derived actors via `level.place_actor`

**Working value** for `actorClass` in this run: `/Game/_Dev/MCP/BP_MCP_SmokeTest.BP_MCP_SmokeTest_C` (generated class path with `_C` suffix).

If `place_actor` fails, verify the Blueprint compiled and try the `_C` object path from the Content Browser / asset tool output.

---

## Acceptance criteria (TH-511)

| Criterion | Met |
| --- | --- |
| UE-MCP can read known ALS package paths | Yes (`/ALS/...` reads succeeded). |
| UE-MCP can create, mutate, compile, and read back a disposable Blueprint | Yes (after folder + path workarounds). |
| UE-MCP can place or verify the test actor in a level | Yes (playground map + outliner verification). |
| Gaps documented for future agents | Yes (this file). |

---

## Agent discipline ( recap )

- Start every session with `project(action="get_status")`.
- Follow **read → mutate → compile** for Blueprints.
- Prefer native UE-MCP actions over `editor(execute_python)`; use Python only as an escape hatch and consider `feedback` for native tool gaps.
