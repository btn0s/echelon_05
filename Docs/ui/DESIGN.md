---
design_system:
  name: Echelon Stealth Instrument UI
  version: 0.1.0
  source: Docs/ui/refs
  intent: Ref-only visual direction for the stealth prototype HUD, terminals, markers, tactical views, and debug overlays.
  exclusions:
    - Existing project widgets
    - Existing HUD assets
    - Existing UI implementation details
    - ALS plugin UI assets
references:
  - file: Docs/ui/refs/36387051-4e6e-41ac-ace2-0e261b66eb7d.jpg
    role: Primary monochrome HUD structure, progress, status cards, grid divisions, outlined actions.
  - file: Docs/ui/refs/565a6096-89df-44d9-8ed4-811e1ce85f56.jpg
    role: Green comms, waveform cards, signal list rows, diegetic terminal mood.
  - file: Docs/ui/refs/11c6190c-148f-49bb-8539-43ca81cd1197.jpg
    role: Dense diagnostics, charts, vertical meters, icon banks, grid plots.
  - file: Docs/ui/refs/9f2863be-c224-4e15-9233-0286b231935b.jpg
    role: Tactical node-map, floating panels, anchors, thin connectors, wide framed canvas.
tokens:
  color:
    field: "#000000"
    panel: "#050505"
    panelMuted: "#0A0A0A"
    linePrimary: "#F2F2F2"
    lineSecondary: "#A8A8A8"
    lineMuted: "#505050"
    textPrimary: "#F4F4F4"
    textSecondary: "#B8B8B8"
    textMuted: "#6A6A6A"
    inverseFill: "#F4F4F4"
    inverseText: "#000000"
    signalGreen: "#70D978"
    signalGreenDim: "#2F6F39"
    transparent: "rgba(0,0,0,0)"
  opacity:
    hairlineIdle: 0.28
    hairlineActive: 0.72
    panelBacking: 0.72
    gridIdle: 0.12
    disabled: 0.22
  typography:
    family: "monospace"
    casing: uppercase
    tracking: 0.08em
    sizes:
      micro: 9
      label: 11
      body: 13
      readout: 24
      banner: 18
  spacing:
    unit: 4
    xs: 4
    sm: 8
    md: 12
    lg: 16
    xl: 24
    screenMargin: 32
  radius:
    none: 0
  stroke:
    hairline: 1
    active: 1
    alert: 2
  motion:
    blinkSlowMs: 1400
    flashMs: 180
    meterStepMs: 80
    alertPulseMs: 420
components:
  hudCard:
    shape: rectangular
    border: 1px low-opacity white
    fill: transparent or near-black only for readability
    label: all-caps monospace
  segmentedMeter:
    direction: horizontal by default
    fill: discrete rectangular segments
    peak: narrow tick marker
  waveform:
    stroke: thin vertical bars or line trace
    color: white for HUD, green only for signal/comms contexts
  banner:
    safe: hidden
    suspicion: bracketed outline with segmented fill
    alert: inverted white block with black text
  worldMarker:
    shape: corner brackets plus short label
    behavior: distance-dimmed, context-only
---

# Stealth UI Design Direction

## Purpose

This document defines the visual target for the stealth prototype UI using only the reference images in `Docs/ui/refs/`. It does not derive from existing widgets, existing HUD assets, or current implementation details.

The UI should feel like a covert surveillance workstation projected over a dark field operation: quiet, severe, technical, and useful under pressure. It should read as instrumentation, not a game overlay. The player should be able to understand detection, visibility, noise, objective state, and extraction state without the interface becoming a debug dashboard.

This follows the `DESIGN.md` pattern described by Stitch: machine-readable tokens in YAML front matter, followed by human-readable rationale and application rules in Markdown. Treat this file as the UI source of truth until a later visual design pass replaces it.

## Reference Read

The four reference images establish a compact but consistent language.

### Green Comms Console

The green comms reference uses a black field, low-brightness green type, waveform cards, list rows, tiny transport icons, status toggles, and dense signal metadata. Its strongest ideas are:

- audio and noise can be visualized as thin waveform strips
- selected or urgent items can be framed with a bright outline instead of a filled panel
- small checkmarks, arrows, toggles, and square indicators can carry state without large labels
- inactive content can fall far into darkness while active content glows just enough to read

This is the best reference for diegetic computer screens, sound-source feedback, radio-like objective terminals, and optional noise visualizations. It should not make the entire always-on HUD green by default.

### Monochrome Transfer Panel

The transfer-progress reference is the cleanest HUD layout source. It uses a centered rectangular grid, thin dividers, all-caps monospace labels, segmented progress bars, a radial progress indicator, large numeric readouts, and outlined buttons. Its strongest ideas are:

- the UI can be readable with only black, white, gray, and line weight
- progress should be segmented and measurable rather than soft or decorative
- large numbers should be reserved for the highest-priority fact
- panel structure can be strict and rectangular without feeling heavy
- buttons and actions should be outlined, not glossy or filled

This is the primary reference for the main gameplay HUD and mission status treatment.

### Dense Diagnostic Instrumentation

The instrumentation reference shows charts, vertical meters, waveform traces, icon banks, grid plots, contour maps, and small labels arranged on a black technical surface. Its strongest ideas are:

- debug and analysis views can be dense if they remain gridded and disciplined
- tick marks, axis labels, small legends, and narrow meters make values feel measurable
- icons should be simple filled or outlined symbols, not illustrative art
- multiple related values can share one visual grammar through repeated bars and charts

This is the primary reference for developer diagnostics, tuning views, and advanced tactical overlays.

### Node-Map Tactical View

The node-view reference uses floating panels connected by thin lines inside a wide framed canvas. The panels are small, square, monochrome, and data-heavy. The strongest ideas are:

- tactical information should float in space with minimal connective lines
- the center can remain mostly empty while still feeling purposeful
- selected targets can be framed by brackets, dots, and line anchors
- full-screen views should feel like looking into a surveillance graph, not opening a modern app

This is the best reference for pause-map, objective-network, guard-awareness, or debug relationship views.

## Design Pillars

### 1. Black Field, Thin Signal

The references are dominated by negative space. The stealth UI should respect darkness and only draw the minimum amount of signal required to support play. Most HUD elements should be linework, text, ticks, or meter segments over transparent or near-black backing.

### 2. Instrumentation Over Decoration

Every element should answer one of these questions:

- Am I being seen?
- Am I making noise?
- What state is the enemy in?
- Is the objective complete?
- Can I extract?

If a visual element does not help answer one of those questions, remove it.

### 3. Monochrome First

The main gameplay HUD should primarily use black, white, and gray. Green belongs to diegetic terminal screens, signal/comms panels, or intentionally isolated noise/audio moments. Color should be a special mode, not the default style.

### 4. Density Is Contextual

The always-on HUD should be sparse. Diagnostic, map, terminal, or pause views can be denser because the player has chosen to inspect them. Dense views must still use the same line weight, type scale, rectangular framing, and restrained color rules.

### 5. Alert Breaks The System

Calm and suspicious states should be quiet: dim outlines, measured pulses, and growing meter density. Full alert is the one state allowed to invert, flash, or use a hard filled block. That exception should be rare so it lands.

## Palette

### Main HUD

- Field: transparent, pure black, or black at low opacity only when readability requires it.
- Primary text and lines: cold white at high brightness.
- Secondary text: medium gray.
- Tertiary metadata: dark gray, barely present.
- Hairlines: white at low opacity.
- Active meter fill: white segmented fill.
- Recent peak marker: narrow white or gray tick.
- Alert: white block, inverted black text, or brief high-brightness pulse.

### Signal / Terminal Green

- Use green for diegetic comms, audio signal displays, and world terminals.
- Keep green low-saturation and low-to-medium brightness.
- Green can brighten for selected states, active waveform playback, or confirmed signal lock.
- Do not use green as the default player HUD color unless the entire HUD is intentionally re-art-directed.

### Forbidden Palette Moves

- No broad red/yellow/green traffic-light UI for stealth state.
- No blue sci-fi hologram wash.
- No warm RPG parchment or military-tan panels.
- No saturated modern app accents.

## Typography

Use a utilitarian monospace or narrow technical face. The references favor type that feels machine-generated and slightly cold.

Recommended treatment:

- all-caps labels
- short abbreviations: `VIS`, `NSE`, `OBJ`, `EXT`, `ALRT`, `SIG`
- tiny metadata text for secondary facts
- one larger numeric value per card at most
- generous letter spacing for headers
- compact line height for diagnostic rows

Avoid prose-heavy HUD copy. Prefer `EXT AVAIL` over `Extraction point is now available`. Prefer `NOISE 42` over `You are making some noise`.

## Lines, Shapes, And Texture

The shape language should be built from simple technical primitives:

- 1 px outlines
- corner brackets
- thin rectangular frames
- segmented horizontal bars
- narrow vertical meter stacks
- radial tick arcs for progress or scanning
- waveform strips for sound
- square status toggles
- tiny dots and line anchors for node connections
- grid lines at very low opacity for diagnostic views

Avoid rounded cards, drop shadows, blur-heavy glass panels, glossy buttons, thick borders, decorative frames, and illustrated icons.

Texture should come from measurement marks and data density, not from grunge overlays.

## Information Architecture

The UI should have three layers.

### Layer 1: Always-On Gameplay HUD

This layer is minimal and should stay near screen edges. It exists only to support moment-to-moment stealth decisions.

Required information:

- visibility exposure
- noise emission
- enemy suspicion or alert state
- objective state
- extraction state
- current stance or movement noise class if useful

The center of the screen should stay clear for reading light, shadow, patrol routes, and interaction targets.

### Layer 2: Contextual World Markers

World markers should be small bracketed tags, not floating game icons. They should identify objects only when needed.

Targets:

- objective marker: `OBJ`
- extraction marker: `EXT`
- guard suspicion marker: small meter or tick cluster
- noise/lure event: brief ring or waveform pulse
- interactable terminal: bracket and short label

Markers should dim with distance or irrelevance. They should never replace environmental reading.

### Layer 3: Inspect / Debug / Terminal Views

This layer may become dense. It should follow the diagnostic and node-map references:

- framed black canvas
- floating square panels
- thin connection lines
- miniature charts
- rows of compact metadata
- waveform and signal cards
- strict rectangular grids

Use this layer for debug causality, tactical analysis, terminal interaction, or map-like views.

## Main HUD Layout

Recommended composition:

- Top left: compact mission block.
- Top center: suspicion or alert banner, hidden or nearly invisible when safe.
- Bottom right: paired `VIS` and `NSE` metric cards.
- Bottom left: optional stance/movement state if it helps play.
- Screen edge: alert pulse only during full alert.

The HUD should feel like separate floating instruments rather than one large cockpit frame.

## Component Contracts

These contracts are the practical bridge between the reference mood and implementation. If a UI element needs a treatment, start here before inventing a new one.

### Technical Card

Use for `VIS`, `NSE`, mission state, extraction state, and compact terminal summaries.

- rectangular frame, no corner radius
- transparent fill by default, near-black fill only when the level behind it harms readability
- 1 px white or gray outline
- one all-caps label in the upper-left
- one primary value or state
- optional segmented meter, waveform, or two-row metadata stack
- no more than three hierarchy levels inside a card

### Segmented Meter

Use for visibility exposure, noise level, suspicion, extraction progress, and objective progress.

- discrete rectangular segments, not a smooth gradient bar
- empty segments are outlines or very dim fill
- active segments brighten from muted gray toward white
- danger is shown through fill count, brightness, and pulse frequency
- recent peaks use a single narrow tick that decays more slowly than the main value

### Waveform Strip

Use for noise, comms, terminals, and debug event history.

- quiet state is nearly flat and low opacity
- transient sounds create vertical spikes
- sustained sounds create repeated bars or a thin oscillating trace
- HUD waveform is monochrome
- terminal waveform may use signal green
- decorative looping waveform is not allowed unless it maps to real state

### Bracket Marker

Use for objectives, extraction, interactables, guards, and selected tactical nodes.

- four corner brackets or two opposing bracket corners
- short label such as `OBJ`, `EXT`, `SIG`, `GUARD`, or `DOOR`
- optional one-line state text when close or selected
- no filled icon bubble
- no large exclamation mark
- fade by distance and relevance

### Alert Banner

Use only when enemy state matters to immediate play.

- hidden while safe
- bracketed and dim for early suspicion
- segmented fill for escalating suspicion
- inverted block only for confirmed alert
- alert text must be short: `CONTACT`, `SUSPICION`, `INVESTIGATING`, or `ALERT`

## Layout Grammar

### Safe Screen

The safe state is mostly empty. The player should feel the level, lighting, patrol bodies, and objective space more than the interface.

- mission block stays top-left
- `VIS` and `NSE` stay bottom-right or near a lower corner
- no center banner
- no persistent cross-screen frame
- world markers appear only for nearby or important objects

### Pressure Screen

When the player is nearly detected, the UI can become sharper but should not become colorful.

- detection banner appears at top-center
- `VIS` meter brightens first for visual exposure
- `NSE` waveform spikes first for sound exposure
- guard marker can show tick count or bracket pulse
- objective and extraction markers remain secondary

### Inspection Screen

Terminal, map, pause, or debug screens can become dense because the player has opted into inspection.

- use a full or partial black canvas
- divide the canvas with strict rectangular frames
- place data cards as floating panels
- connect related nodes with thin lines and dot anchors
- reserve signal green for comms/audio surfaces
- keep text compact and all-caps

## Accessibility And Readability

This art direction is intentionally low-color, so readability must come from contrast, shape, rhythm, and placement.

- do not rely on hue alone to communicate danger or completion
- every state needs a shape or text difference
- alert inversion must be readable against dark and bright gameplay backgrounds
- small labels should remain legible at 1080p
- meters should still read when viewed peripherally
- green terminal content should meet contrast against black wherever it carries required interaction text

## Content Rules

### Preferred Labels

Use compact tactical labels:

- `MISSION`
- `OBJ ACTIVE`
- `OBJ COMPLETE`
- `EXT LOCKED`
- `EXT AVAIL`
- `VIS DARK`
- `VIS EXPOSED`
- `NSE SILENT`
- `NSE LOUD`
- `CONTACT`
- `SUSPICION`
- `INVESTIGATING`
- `ALERT`
- `SIGNAL`
- `TRACE`

### Copy Tone

The UI should sound like a system, not a narrator. Prefer short state phrases over explanatory sentences.

- Use `EXT AVAIL`, not `Extraction point is now available`.
- Use `NSE LOUD`, not `You are making a lot of noise`.
- Use `OBJ COMPLETE`, not `Objective completed successfully`.
- Use `SIGNAL LOST`, not `The signal has been disconnected`.

### Mission Block

Visual source: monochrome transfer panel and node-map labels.

Structure:

- small top label: `MISSION`
- row 1: square indicator plus `OBJ ACTIVE` or `OBJ COMPLETE`
- row 2: square indicator plus `EXT LOCKED`, `EXT AVAIL`, or `EXT USED`
- optional row 3 after failure/escalation: `COMPROMISED`

Treatment:

- inactive square is outline-only
- active square is filled or brighter
- completed row gets a single brief flash, then settles
- block remains small and quiet

### Detection Banner

Visual source: transfer panel action area and node-map selected frames.

States:

- safe: hidden
- curious: thin bracket frame, dim text, slow pulse
- suspicious: brighter frame, segmented fill, slightly faster pulse
- investigating: framed label plus directional tick or small scanning arc
- alert: inverted white bar, hard label, screen-edge pulse

Suggested labels:

- `UNSEEN`
- `CONTACT`
- `SUSPICION`
- `INVESTIGATING`
- `ALERT`

Avoid large warning colors. Alert can be violent through inversion and brightness alone.

### VIS Card

Visual source: transfer segmented bar and diagnostic vertical meters.

Purpose: show how exposed the player is to sight.

Structure:

- header: `VIS`
- main readout: percentage, tier, or compact numeric value
- segmented horizontal bar
- optional peak tick
- tiny state label: `DARK`, `PARTIAL`, `LIT`, or `EXPOSED`

Behavior:

- low values are faint
- segment brightness rises with danger
- peak tick decays slowly
- rapid changes should step through segments rather than animate smoothly like a mobile app

### NSE Card

Visual source: green waveform cards and diagnostic meters.

Purpose: show how much sound the player is emitting.

Structure:

- header: `NSE`
- main readout: percentage, tier, or compact numeric value
- waveform strip or segmented bar
- short decay trail for recent loud sound
- tiny state label: `SILENT`, `SOFT`, `AUDIBLE`, or `LOUD`

Behavior:

- footsteps create small waveform spikes
- loud events briefly brighten the strip
- sustained noise fills the meter in a measured way
- optional green may be used only for a signal/waveform accent, not the whole card

### Stance / Movement Readout

Visual source: comms toggles and transfer metadata.

This should be tiny and secondary. It can sit under the `VIS` / `NSE` cards or bottom left.

Suggested format:

- `STANCE CROUCH`
- `MOVE SLOW`
- `SURFACE TILE`

Only show facts that help the player understand visibility or noise. Do not turn this into a full locomotion debug panel.

## World-Space Markers

World markers should use node-map brackets and small technical labels.

### Guard Suspicion Plate

- small rectangular plate near the guard or over the head
- no plate when unaware unless needed for tuning
- one to five ticks for suspicion
- hard inverted `ALERT` label only in full alert
- no large floating exclamation mark

### Objective Marker

- corner bracket around the object or a nearby anchor point
- label: `OBJ`
- state text appears only when close: `ACTIVE`, `READY`, or `DONE`
- completed state flashes once then dims

### Extraction Marker

- label: `EXT`
- locked state is dim outline
- available state is brighter outline with slow pulse
- used state fades out or switches to `EXIT`

### Noise Event Pulse

- short radial ring or waveform burst at the source
- very brief lifetime
- brightness based on loudness
- optional green only if the moment is explicitly treated as signal feedback

## Terminal / Diegetic Screen Direction

Terminals can lean harder into the green comms reference than the normal HUD.

Use:

- black background
- green linework
- waveform cards
- message rows
- tiny transport controls
- selector frames
- all-caps metadata
- timestamps, unknown source labels, signal names

Possible labels:

- `INCOMING SIGNAL`
- `FREQUENCY`
- `TRACE`
- `DIVERT`
- `FLAG`
- `REPLAY`
- `UPLINK`

Terminal screens can be denser and more atmospheric because they are diegetic interactions, not always-on combat information.

## Debug / Diagnostic Direction

Debug views should borrow from the dense instrumentation reference instead of from a developer console.

Recommended elements:

- right-side diagnostic stack
- graph strips for visibility and noise over time
- vertical meter columns for perception inputs
- small axis labels and tick marks
- current guard state row
- sight/noise event log
- black panels with thin white dividers
- optional node links between player, guard, objective, and extraction

Debug data can be raw and numerical, but it should still look like part of the same art direction. The player-facing HUD should not expose raw tuning values by default.

## Motion Rules

Motion should be sparse and readable.

Use:

- slow blink for live indicators
- brief flash on objective/extraction state changes
- waveform spikes for sound
- segmented meter stepping for exposure and noise
- slow scan arc for investigation
- hard edge pulse for full alert

Avoid:

- constant idle shimmer
- noisy animated backgrounds
- looping decorative waveforms with no gameplay meaning
- smooth elastic app-style transitions
- overactive warning flashes before full alert

## State Language

### Safe

- most HUD elements dim or hidden
- no banner
- meters faint
- mission block remains readable

### Curious

- thin bracket or small tick appears
- low-frequency pulse
- suspicion meter begins filling
- no inversion

### Suspicious / Investigating

- banner appears
- segmented fill becomes clearer
- small scan arc or directional tick can appear
- guard marker may show ticks

### Alert

- inverted banner or solid white block
- screen-edge pulse
- high-contrast label
- optional brief audio waveform spike if alert was caused by sound
- return to quieter language only after state truly drops

## Iconography

Icons should be symbolic and geometric:

- eye or aperture shape for visibility
- waveform for noise
- square checkbox for objective
- bracketed doorway or arrow for extraction
- small triangle/play icon for signal playback
- dot-node connection for tactical relationship

Do not use illustrative character portraits, colorful badges, badge-like achievements, large exclamation marks, or skeuomorphic equipment icons.

## Scale And Spacing

The references use strict alignment and generous empty space. Keep the HUD compact.

Guidelines:

- keep margins consistent on each edge
- align cards to a shared grid
- use narrow gutters between rows
- prefer small cards over large panels
- leave the crosshair/center view open
- do not let diagnostic density leak into the main HUD

At 1080p, the always-on HUD should feel like small instrumentation in the corners. At higher resolutions, scale linearly enough to remain readable but do not make it luxurious or oversized.

## Example Screen Compositions

### Normal Play

- Top left: `MISSION` block with `OBJ ACTIVE` and `EXT LOCKED`.
- Bottom right: `VIS` and `NSE` cards, both dim.
- No detection banner.
- No world marker unless the objective or extraction point is relevant.

### Near Detection

- Top center: `SUSPICION` bracket banner with segmented fill.
- Bottom right: `VIS` brightens and shows peak tick.
- Guard plate shows two or three suspicion ticks.
- Center remains clear.

### Loud Noise Event

- `NSE` card waveform spikes.
- Brief ring appears at the sound source.
- Detection banner stays quiet unless the enemy reacts.
- Debug layer, if enabled, may log the sound event in a diagnostic row.

### Objective Complete

- Mission block flashes `OBJ COMPLETE` once.
- `EXT LOCKED` switches to `EXT AVAIL`.
- Extraction marker brightens with a slow pulse.
- No celebratory color burst.

### Full Alert

- Top center hard inverted `ALERT` banner.
- Screen edge pulses white.
- Guard plate becomes high-contrast.
- `VIS` and `NSE` can remain visible, but the alert state dominates.

## Acceptance Criteria

Use these checks when reviewing any UI built from this direction.

- The screen still reads as mostly black negative space during normal play.
- The always-on HUD communicates mission, extraction, visibility, noise, and enemy awareness without debug-only raw values.
- `VIS` and `NSE` are visually distinct but clearly part of the same instrument family.
- Suspicion escalation is understandable in monochrome.
- Full alert is the only state that uses a hard inversion or aggressive pulse.
- Objective and extraction markers use bracket language instead of large game icons.
- Terminal or comms screens are the only places where green dominates.
- Dense diagnostic views stay gridded, charted, and technical rather than becoming console text dumps.
- No element appears to be derived from existing project widgets or current UI implementation.
- No project-side UI work requires editing `Plugins/ALS-Refactored`.

## Do / Do Not

Do:

- Use black negative space as the primary canvas.
- Use thin linework, compact all-caps type, and segmented meters.
- Encode danger through brightness, density, fill amount, and inversion.
- Keep the always-on HUD sparse.
- Use waveform language for noise and comms.
- Use node-map language for tactical/debug relationships.
- Let green live mostly in terminals and signal displays.
- Make objective and extraction state readable at a glance.

Do not:

- Base the direction on existing widgets or current UI implementation.
- Fill the gameplay HUD with green because one reference is green.
- Use broad traffic-light color coding.
- Add decorative panels that obscure the level.
- Show raw simulation values in the main HUD.
- Use rounded modern app cards, glossy controls, or heavy drop shadows.
- Let debug density become the default player experience.
- Edit ALS plugin UI assets to achieve this direction.
