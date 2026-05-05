// Player HUD overlay for the Echelon stealth prototype.
//
// Structure follows Docs/ui/DESIGN.md (`Information Architecture`,
// `Main HUD Layout`, `Component Contracts`, `State Language`).
//
// Layer composition (top-down draw order):
//   1. Edge alert vignette        (Alert state only)
//   2. Detection banner           (top-center, hidden when safe)
//   3. Mission block              (top-left)
//   4. Stance / surface readout   (bottom-left, optional)
//   5. Interaction prompt         (bottom-center, when focused)
//   6. VIS + NSE technical cards  (bottom-right)
//   7. Diagnostics overlay        (right side, gated by stealth.DebugDraw)

#include "Stealth/UI/StealthHUDWidget.h"

#include "Stealth/Components/StealthGuardBrainComponent.h"
#include "Stealth/Components/StealthHealthComponent.h"
#include "Stealth/Components/StealthInteractorComponent.h"
#include "Stealth/Subsystems/StealthSimulationSubsystem.h"
#include "Stealth/UI/StealthUIPrimitives.h"
#include "Stealth/UI/StealthUITokens.h"

#include "Engine/World.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"

namespace
{
	using namespace StealthUI;

	// ── Sampling constants ───────────────────────────────────────────────────
	constexpr float NoiseHistorySamples = 48.f;     // ~3.8 s at 80 ms / sample
	constexpr float NoiseSampleStep     = 0.080f;   // matches design token meterStepMs
	constexpr float PeakDecayPerSecond  = 0.30f;    // visible memory ~3 s before fully draining

	// ── Card geometry — one place to retune the metric cluster ───────────────
	constexpr float CardW   = 188.f;
	constexpr float CardH   = 84.f;
	constexpr float CardGap = SpacingMD;

	// ── Mission row geometry ─────────────────────────────────────────────────
	constexpr float MissionIndicatorSize = 9.f;
	constexpr float MissionRowHeight     = 18.f;
	constexpr float MissionLabelXOffset  = 18.f;
	constexpr float MissionStateXOffset  = 56.f;

	// ── Detection banner geometry ────────────────────────────────────────────
	constexpr float BannerW = 300.f;
	constexpr float BannerH = 32.f;

	// ── Helpers ──────────────────────────────────────────────────────────────

	/** Map normalized visibility to the design-canonical state label. */
	const TCHAR* VisStateLabel(float V01)
	{
		if (V01 < 0.20f) { return TEXT("DARK"); }
		if (V01 < 0.50f) { return TEXT("PARTIAL"); }
		if (V01 < 0.80f) { return TEXT("LIT"); }
		return TEXT("EXPOSED");
	}

	/** Map normalized noise to the design-canonical state label. */
	const TCHAR* NoiseStateLabel(float N01)
	{
		if (N01 < 0.10f) { return TEXT("SILENT"); }
		if (N01 < 0.40f) { return TEXT("SOFT"); }
		if (N01 < 0.75f) { return TEXT("AUDIBLE"); }
		return TEXT("LOUD");
	}

	/** Suspicion state -> short banner label. */
	const TCHAR* SuspicionLabel(EGuardSuspicionState S)
	{
		switch (S)
		{
		case EGuardSuspicionState::Curious:        return TEXT("CONTACT");
		case EGuardSuspicionState::Suspicious:     return TEXT("SUSPICION");
		case EGuardSuspicionState::Investigating:  return TEXT("INVESTIGATING");
		case EGuardSuspicionState::Alert:          return TEXT("ALERT");
		default:                                    return TEXT("UNSEEN");
		}
	}

	/** Stance + locomotion -> single short readout, e.g. "STANCE CROUCH  MOVE WALK". */
	FString StanceMoveLine(const FStealthMovementState& Mv)
	{
		const TCHAR* StanceTxt = (Mv.Stance == EStealthStance::Crouching) ? TEXT("CROUCH") : TEXT("STAND");

		const TCHAR* LocoTxt = TEXT("IDLE");
		switch (Mv.Locomotion)
		{
		case EStealthLocomotion::Walk:   LocoTxt = TEXT("WALK");   break;
		case EStealthLocomotion::Run:    LocoTxt = TEXT("RUN");    break;
		case EStealthLocomotion::Sprint: LocoTxt = TEXT("SPRINT"); break;
		default: break;
		}
		return FString::Printf(TEXT("STANCE %s    MOVE %s"), StanceTxt, LocoTxt);
	}

	/** Decay a peak marker toward the current value at PeakDecayPerSecond / s. */
	void TickPeak(float& Peak, float Current, float Dt)
	{
		Peak = FMath::Max(Peak, Current);
		Peak = FMath::Max(Current, Peak - PeakDecayPerSecond * Dt);
	}

	template <typename TEnum>
	FString EnumDisplay(TEnum V)
	{
		if (const UEnum* E = StaticEnum<TEnum>())
		{
			return E->GetDisplayNameTextByValue(static_cast<int64>(V)).ToString().ToUpper();
		}
		return FString::FromInt(static_cast<int32>(V));
	}

	/** Measured pixel width of a string at a font size (used to right-align readouts). */
	float MeasureTextWidth(const FString& Str, const FSlateFontInfo& Font)
	{
		const TSharedRef<FSlateFontMeasure> Measurer = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		return static_cast<float>(Measurer->Measure(Str, Font).X);
	}

	// ── Composite components ─────────────────────────────────────────────────

	/**
	 * VIS technical card — DESIGN.md `### VIS Card`.
	 *
	 * Layout (top to bottom):
	 *   header label `VIS`              (micro, muted)
	 *   percentage readout              (readout, primary)
	 *   segmented meter + peak tick     (bottom)
	 *   state label `DARK/PARTIAL/...`  (micro, secondary, right-aligned)
	 */
	void DrawVisCard(FSlateWindowElementList& Out, int32& Layer, const FGeometry& Geo,
		float X, float Y, float W, float H, float V01, float Peak01)
	{
		// Header
		DrawText(Out, Layer, Geo, TEXT("VIS"), {X, Y}, TextMuted(), FMicro());

		// Readout — large numeric, right-aligned to card edge so it visually
		// terminates the card and so the eye lands on the value first.
		const FString Value = FString::Printf(TEXT("%2.0f"), V01 * 100.f);
		const float ValueW  = MeasureTextWidth(Value, FReadout());
		DrawText(Out, Layer, Geo, Value, {X + W - ValueW, Y - 2.f}, TextPrimary(), FReadout());

		// Tiny "%" suffix sitting in the readout's lower-right baseline.
		DrawText(Out, Layer, Geo, TEXT("%"), {X + W - 8.f, Y + 18.f}, TextSecondary(), FMicro());

		// Segmented meter
		const float MeterY = Y + H - 18.f;
		SegmentedMeter(Out, Layer, Geo, X, MeterY, W, 8.f, V01, /*Segments*/ 16, Peak01);

		// State label
		const FString State = VisStateLabel(V01);
		const float StateW  = MeasureTextWidth(State, FMicro());
		DrawText(Out, Layer, Geo, State, {X + W - StateW, MeterY + 10.f}, TextSecondary(), FMicro());
	}

	/**
	 * NSE technical card — DESIGN.md `### NSE Card`.
	 *
	 * Same layout as VIS but the bottom slot is a waveform strip rather than
	 * a segmented bar. The waveform pulls from the widget-owned circular history.
	 */
	void DrawNoiseCard(FSlateWindowElementList& Out, int32& Layer, const FGeometry& Geo,
		float X, float Y, float W, float H, float N01, const TArray<float>& History)
	{
		DrawText(Out, Layer, Geo, TEXT("NSE"), {X, Y}, TextMuted(), FMicro());

		const FString Value = FString::Printf(TEXT("%2.0f"), N01 * 100.f);
		const float ValueW  = MeasureTextWidth(Value, FReadout());
		DrawText(Out, Layer, Geo, Value, {X + W - ValueW, Y - 2.f}, TextPrimary(), FReadout());

		DrawText(Out, Layer, Geo, TEXT("%"), {X + W - 8.f, Y + 18.f}, TextSecondary(), FMicro());

		const float WaveY = Y + H - 22.f;
		WaveformStrip(Out, Layer, Geo, X, WaveY, W, 12.f, History,
			LinePrimary());

		const FString State = NoiseStateLabel(N01);
		const float StateW  = MeasureTextWidth(State, FMicro());
		DrawText(Out, Layer, Geo, State, {X + W - StateW, WaveY + 14.f}, TextSecondary(), FMicro());
	}

	/**
	 * Detection banner — DESIGN.md `### Detection Banner`.
	 *
	 * Five distinct visual states all sit in the same screen real estate so
	 * the player learns "danger lives here" once and never re-scans.
	 *
	 *   Unaware       hidden
	 *   Curious       corner brackets, dim text, slow blink
	 *   Suspicious    full outline, segmented fill, brighter text
	 *   Investigating outline + scan arc + label
	 *   Alert         inverted white block, black text (the only inversion on the HUD)
	 */
	void DrawDetectionBanner(FSlateWindowElementList& Out, int32& Layer, const FGeometry& Geo,
		float ScreenW, float T, EGuardSuspicionState State, float Susp01)
	{
		if (State == EGuardSuspicionState::Unaware) { return; }

		const float BX = (ScreenW - BannerW) * 0.5f;
		const float BY = ScreenMargin;

		const FString Label   = SuspicionLabel(State);
		const FString Percent = FString::Printf(TEXT("%2.0f%%"), Susp01 * 100.f);

		switch (State)
		{
		case EGuardSuspicionState::Curious:
		{
			// Slow blink: 1400 ms → ~0.7 Hz. Brackets only, never the full frame.
			const float Pulse = 0.55f + 0.25f * FMath::Sin(T * (TWO_PI / 1.4f));
			const FLinearColor C = LinePrimary().CopyWithNewOpacity(Pulse * 0.55f);

			CornerBrackets(Out, Layer, Geo, BX, BY, BannerW, BannerH, 10.f, C);
			DrawText(Out, Layer, Geo, Label,
				{BX + SpacingLG, BY + 9.f}, TextSecondary(), FLabel());
			break;
		}
		case EGuardSuspicionState::Suspicious:
		{
			// Outlined frame + segmented fill across the bottom rail of the banner.
			Outline(Out, Layer, Geo, BX, BY, BannerW, BannerH, HairlineActive());
			SegmentedMeter(Out, Layer, Geo, BX + 4.f, BY + BannerH - 6.f, BannerW - 8.f, 3.f,
				Susp01, /*Segments*/ 22);
			DrawText(Out, Layer, Geo, Label,
				{BX + SpacingLG, BY + 8.f}, TextPrimary(), FLabel());
			DrawText(Out, Layer, Geo, Percent,
				{BX + BannerW - SpacingLG - MeasureTextWidth(Percent, FMicro()), BY + 10.f},
				TextSecondary(), FMicro());
			break;
		}
		case EGuardSuspicionState::Investigating:
		{
			// Outlined frame + segmented fill + sweeping scan arc on the left side
			// to encode "actively searching" at a glance.
			Outline(Out, Layer, Geo, BX, BY, BannerW, BannerH, HairlineActive());
			SegmentedMeter(Out, Layer, Geo, BX + 4.f, BY + BannerH - 6.f, BannerW - 8.f, 3.f,
				Susp01, /*Segments*/ 22);

			const float Sweep = FMath::Fmod(T * 2.4f, PI * 2.f);
			ScanArc(Out, Layer, Geo, BX + 16.f, BY + BannerH * 0.5f, 9.f,
				Sweep, PI * 0.65f, LinePrimary().CopyWithNewOpacity(0.85f), 14);

			DrawText(Out, Layer, Geo, Label,
				{BX + 36.f, BY + 8.f}, TextPrimary(), FLabel());
			DrawText(Out, Layer, Geo, Percent,
				{BX + BannerW - SpacingLG - MeasureTextWidth(Percent, FMicro()), BY + 10.f},
				TextSecondary(), FMicro());
			break;
		}
		case EGuardSuspicionState::Alert:
		{
			// THE inversion. Solid white block, black text. Slow heartbeat pulse on
			// alpha so it reads as "hot" without becoming a mobile-app shimmer.
			const float Pulse = 0.92f + 0.08f * FMath::Sin(T * (TWO_PI / 0.42f));
			FillRect(Out, Layer++, Geo, {BX, BY}, {BannerW, BannerH + 2.f},
				FLinearColor(Pulse, Pulse, Pulse, 1.f));
			DrawText(Out, Layer, Geo, Label,
				{BX + SpacingLG, BY + 6.f}, InverseText(), FBanner());
			DrawText(Out, Layer, Geo, Percent,
				{BX + BannerW - SpacingLG - MeasureTextWidth(Percent, FBody()), BY + 10.f},
				FLinearColor(0.f, 0.f, 0.f, 0.6f), FBody());
			break;
		}
		default:
			break;
		}
	}

	/** Screen-edge alert vignette — used only when full alert is active. */
	void DrawAlertVignette(FSlateWindowElementList& Out, int32& Layer, const FGeometry& Geo,
		float ScreenW, float ScreenH, float T)
	{
		const float Pulse = 0.10f + 0.10f * FMath::Sin(T * (TWO_PI / 0.42f));
		const FLinearColor V = LinePrimary().CopyWithNewOpacity(Pulse);
		constexpr float EdgeW = 6.f;

		FillRect(Out, Layer++, Geo, {0.f,            0.f},              {ScreenW, EdgeW}, V);
		FillRect(Out, Layer++, Geo, {0.f,            ScreenH - EdgeW},  {ScreenW, EdgeW}, V);
		FillRect(Out, Layer++, Geo, {0.f,            0.f},              {EdgeW, ScreenH}, V);
		FillRect(Out, Layer++, Geo, {ScreenW - EdgeW, 0.f},             {EdgeW, ScreenH}, V);
	}
}

// ─────────────────────────────────────────────────────────────────────────────

int32 UStealthHUDWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	using namespace StealthUI;

	const int32 Base = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements,
		LayerId, InWidgetStyle, bParentEnabled);
	int32 L = Base + 1;

	UWorld* World = GetWorld();
	if (!World) { return L; }
	UStealthSimulationSubsystem* Sim = World->GetSubsystem<UStealthSimulationSubsystem>();
	if (!Sim) { return L; }

	const float ScreenW = static_cast<float>(AllottedGeometry.GetLocalSize().X);
	const float ScreenH = static_cast<float>(AllottedGeometry.GetLocalSize().Y);
	const float T       = World->GetTimeSeconds();

	// ── Snapshot data from the simulation subsystem ──────────────────────────
	const FStealthMovementState Mv  = Sim->GetPlayerMovement();
	const FVisibilityEmitter    Vis = Sim->GetPlayerVisibility();
	const FSoundEmitter         Snd = Sim->GetPlayerSoundEmission();
	const FObjectiveState       Obj = Sim->GetObjectiveState();
	const FExtractionState      Ext = Sim->GetExtractionState();
	const FStealthHealthState   HP  = Sim->GetPlayerHealthState();

	const TArray<TWeakObjectPtr<UStealthGuardBrainComponent>> Brains = Sim->GetRegisteredGuardBrains();
	const UStealthGuardBrainComponent* PrimaryBrain = nullptr;
	for (const TWeakObjectPtr<UStealthGuardBrainComponent>& B : Brains)
	{
		if (B.IsValid()) { PrimaryBrain = B.Get(); break; }
	}
	const FSuspicionState Suspicion = PrimaryBrain ? PrimaryBrain->Suspicion : FSuspicionState();
	const float Susp01 = FMath::Clamp(Suspicion.Value / 100.f, 0.f, 1.f);

	const float Vis01   = FMath::Clamp(Vis.CurrentVisibility, 0.f, 1.f);
	const float Noise01 = FMath::Clamp(Snd.Radius / 1400.f, 0.f, 1.f);

	// ── Render-state book-keeping (peak decay + waveform sampling) ───────────
	const float Dt = (LastSampleTime > 0.f) ? FMath::Max(0.f, T - LastSampleTime) : 0.f;
	TickPeak(PeakVis01,   Vis01,   Dt);
	TickPeak(PeakNoise01, Noise01, Dt);

	// Sample the noise history at a fixed cadence so the waveform reads as data,
	// not as a frame-rate-coupled animation.
	if (NoiseHistory.Num() == 0) { NoiseHistory.Init(0.f, static_cast<int32>(NoiseHistorySamples)); }
	if (LastSampleTime < 0.f || (T - LastSampleTime) >= NoiseSampleStep)
	{
		NoiseHistory.RemoveAt(0, 1, EAllowShrinking::No);
		NoiseHistory.Add(Noise01);
		LastSampleTime = T;
	}

	// One-shot completion flashes for mission rows
	if (Obj.bCompleted && !bLastObjCompleted) { ObjFlashAtTime = T; }
	bLastObjCompleted = Obj.bCompleted;
	if (Ext.bAvailable && !bLastExtAvailable) { ExtFlashAtTime = T; }
	bLastExtAvailable = Ext.bAvailable;

	// ── 1. Edge alert vignette (Alert state only) ────────────────────────────
	if (Suspicion.State == EGuardSuspicionState::Alert)
	{
		DrawAlertVignette(OutDrawElements, L, AllottedGeometry, ScreenW, ScreenH, T);
	}

	// ── 2. Detection banner (top-center) ─────────────────────────────────────
	DrawDetectionBanner(OutDrawElements, L, AllottedGeometry, ScreenW, T, Suspicion.State, Susp01);

	// ── 3. Mission block (top-left) ──────────────────────────────────────────
	{
		const float MX = ScreenMargin;
		float MY = ScreenMargin - SpacingXS;

		DrawText(OutDrawElements, L, AllottedGeometry, TEXT("MISSION"),
			{MX, MY}, TextMuted(), FMicro());
		MY += 14.f;

		// Thin underline tying the section header to its rows.
		HRule(OutDrawElements, L, AllottedGeometry, MX, MY, 110.f);
		MY += 8.f;

		// OBJ row
		{
			const int32 Required  = Sim->GetRequiredObjectiveCount();
			const int32 Completed = Sim->GetCompletedRequiredObjectiveCount();
			const bool  bDone     = Obj.bCompleted;
			const bool  bFlashing = (T - ObjFlashAtTime) < 0.18f;

			const FString State = Required > 1
				? FString::Printf(TEXT("%d/%d"), Completed, Required)
				: (bDone ? TEXT("OBJ COMPLETE") : TEXT("OBJ ACTIVE"));

			IndicatorSquare(OutDrawElements, L, AllottedGeometry,
				MX, MY + 1.f, MissionIndicatorSize, bDone || bFlashing, bFlashing ? 1.f : 0.95f);
			DrawText(OutDrawElements, L, AllottedGeometry, TEXT("OBJ"),
				{MX + MissionLabelXOffset, MY}, TextMuted(), FMicro());
			DrawText(OutDrawElements, L, AllottedGeometry, State,
				{MX + MissionStateXOffset, MY},
				bDone ? TextPrimary() : TextSecondary(), FLabel());
			MY += MissionRowHeight;
		}

		// EXT row
		{
			const bool  bAvail    = Ext.bAvailable;
			const bool  bUsed     = Ext.bUsed;
			const bool  bActive   = bAvail && !bUsed;
			const bool  bFlashing = (T - ExtFlashAtTime) < 0.18f;
			const FString State = bUsed ? TEXT("EXT USED") : (bAvail ? TEXT("EXT AVAIL") : TEXT("EXT LOCKED"));

			IndicatorSquare(OutDrawElements, L, AllottedGeometry,
				MX, MY + 1.f, MissionIndicatorSize, bActive || bUsed || bFlashing,
				bFlashing ? 1.f : (bActive ? 0.95f : 0.7f));
			DrawText(OutDrawElements, L, AllottedGeometry, TEXT("EXT"),
				{MX + MissionLabelXOffset, MY}, TextMuted(), FMicro());
			DrawText(OutDrawElements, L, AllottedGeometry, State,
				{MX + MissionStateXOffset, MY},
				bActive ? TextPrimary() : (bUsed ? TextSecondary() : TextSecondary()),
				FLabel());
			MY += MissionRowHeight;
		}

		// HP row (only when a player health component is registered)
		if (HP.Owner)
		{
			const bool bAlive = HP.VitalState == EStealthVitalState::Alive;
			const FString State = bAlive
				? FString::Printf(TEXT("HP %2.0f/%2.0f"), HP.CurrentHealth, HP.MaxHealth)
				: FString::Printf(TEXT("HP %s"), *EnumDisplay(HP.VitalState));

			IndicatorSquare(OutDrawElements, L, AllottedGeometry,
				MX, MY + 1.f, MissionIndicatorSize, !bAlive, bAlive ? 0.95f : 1.f);
			DrawText(OutDrawElements, L, AllottedGeometry, TEXT("HP"),
				{MX + MissionLabelXOffset, MY}, TextMuted(), FMicro());
			DrawText(OutDrawElements, L, AllottedGeometry, State,
				{MX + MissionStateXOffset, MY},
				bAlive ? TextSecondary() : TextPrimary(), FLabel());
			MY += MissionRowHeight;
		}

		// COMPROMISED — only after a confirmed alert ever fired this run.
		if (Sim->HasAlertOccurred())
		{
			const float Pulse = 0.55f + 0.30f * FMath::Sin(T * 2.5f);
			IndicatorSquare(OutDrawElements, L, AllottedGeometry,
				MX, MY + 1.f, MissionIndicatorSize, false, Pulse);
			DrawText(OutDrawElements, L, AllottedGeometry, TEXT("COMPROMISED"),
				{MX + MissionLabelXOffset, MY}, TextPrimary(), FLabel());
		}
	}

	// ── 4. Stance / surface readout (bottom-left) ────────────────────────────
	{
		const float SX = ScreenMargin;
		const float SY = ScreenH - ScreenMargin - 14.f;

		DrawText(OutDrawElements, L, AllottedGeometry, StanceMoveLine(Mv),
			{SX, SY}, TextSecondary(), FMicro());

		// Live blink dot — encodes "the simulation is ticking" without color.
		if (Blink(T, BlinkSlowMs) < 0.5f)
		{
			FillRect(OutDrawElements, L++, AllottedGeometry,
				{SX - 12.f, SY + 4.f}, {4.f, 4.f},
				LinePrimary().CopyWithNewOpacity(0.85f));
		}
	}

	// ── 5. Interaction prompt (bottom-center, when focused) ──────────────────
	if (const APawn* Pawn = GetOwningPlayerPawn())
	{
		if (const UStealthInteractorComponent* Interactor = Pawn->FindComponentByClass<UStealthInteractorComponent>())
		{
			const FText InteractionText = Interactor->GetFocusedInteractionText();
			if (!InteractionText.IsEmpty())
			{
				const FString Prompt = InteractionText.ToString().ToUpper();
				const float W = FMath::Max(180.f, MeasureTextWidth(Prompt, FLabel()) + 48.f);
				const float H = 26.f;
				const float X = (ScreenW - W) * 0.5f;
				const float Y = ScreenH - 132.f;

				CornerBrackets(OutDrawElements, L, AllottedGeometry, X, Y, W, H, 8.f,
					HairlineActive());
				DrawText(OutDrawElements, L, AllottedGeometry, Prompt,
					{X + (W - MeasureTextWidth(Prompt, FLabel())) * 0.5f, Y + 7.f},
					TextPrimary(), FLabel());
			}
		}
	}

	// ── 6. VIS + NSE technical cards (bottom-right) ──────────────────────────
	{
		const float TotalW = CardW * 2.f + CardGap;
		const float CX     = ScreenW - TotalW - ScreenMargin;
		const float CY     = ScreenH - CardH - ScreenMargin - 18.f;

		DrawVisCard(OutDrawElements, L, AllottedGeometry,
			CX, CY, CardW, CardH, Vis01, PeakVis01);
		DrawNoiseCard(OutDrawElements, L, AllottedGeometry,
			CX + CardW + CardGap, CY, CardW, CardH, Noise01, NoiseHistory);

		// Section header riding above the pair — names the instrument cluster.
		DrawText(OutDrawElements, L, AllottedGeometry, TEXT("EXPOSURE"),
			{CX, CY - 16.f}, TextMuted(), FMicro());
		HRule(OutDrawElements, L, AllottedGeometry, CX + 64.f, CY - 11.f, TotalW - 64.f);
	}

	// ── 7. Diagnostics overlay (right side, gated by stealth.DebugDraw) ──────
	int32 DebugDraw = 0;
	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("stealth.DebugDraw")))
	{
		DebugDraw = CVar->GetInt();
	}

	if (DebugDraw > 0)
	{
		const FStealthAlsDebugSnapshot   Als    = Sim->GetAlsDebugSnapshot();
		const FStealthLightSamplingDebug Ls     = Sim->GetLastLightSamplingDebug();
		const TArray<FStealthSoundEvent> Events = Sim->GetActiveSoundEvents();

		// Diagnostics panel: a black canvas with a thin frame, sectioned rows,
		// and small vertical meter columns in the right gutter — direct read of
		// the dense-instrumentation reference.
		constexpr float DW = 360.f;
		constexpr float DH = 380.f;
		const float DX = ScreenW - DW - ScreenMargin;
		const float DY = ScreenMargin + BannerH + SpacingLG;

		FillRect(OutDrawElements, L++, AllottedGeometry, {DX, DY}, {DW, DH}, PanelBacking());
		Outline(OutDrawElements, L, AllottedGeometry, DX, DY, DW, DH, HairlineActive());

		// Header band
		FillRect(OutDrawElements, L++, AllottedGeometry, {DX, DY}, {DW, 18.f}, Field());
		HRule(OutDrawElements, L, AllottedGeometry, DX, DY + 18.f, DW);
		DrawText(OutDrawElements, L, AllottedGeometry, TEXT("DIAGNOSTICS"),
			{DX + SpacingMD, DY + 4.f}, TextMuted(), FMicro());
		DrawText(OutDrawElements, L, AllottedGeometry, TEXT("STEALTH.DEBUGDRAW"),
			{DX + DW - SpacingMD - MeasureTextWidth(TEXT("STEALTH.DEBUGDRAW"), FMicro()), DY + 4.f},
			TextMuted(), FMicro());

		// Vertical meter column gutter (right side) — VIS / NSE / SUS as small
		// data-bar stacks. Same encoding as the gameplay HUD just in inspect-form.
		auto DrawVMeter = [&](float Cx, float Cy, const FString& Label, float V01)
		{
			DrawText(OutDrawElements, L, AllottedGeometry, Label,
				{Cx, Cy - 10.f}, TextMuted(), FMicro());
			constexpr int32 Steps = 12;
			const int32 ActiveSteps = FMath::CeilToInt(V01 * Steps);
			for (int32 i = 0; i < Steps; ++i)
			{
				const float Sy = Cy + (Steps - 1 - i) * 6.f;
				const bool bOn = i < ActiveSteps;
				const float B = 0.32f + V01 * 0.6f;
				FillRect(OutDrawElements, L++, AllottedGeometry, {Cx, Sy}, {18.f, 4.f},
					bOn ? FLinearColor(B, B, B, 1.f) : LinePrimary().CopyWithNewOpacity(OpacityHairlineIdle));
			}
		};

		DrawVMeter(DX + DW - 70.f, DY + 36.f, TEXT("VIS"), Vis01);
		DrawVMeter(DX + DW - 46.f, DY + 36.f, TEXT("NSE"), Noise01);
		DrawVMeter(DX + DW - 22.f, DY + 36.f, TEXT("SUS"), Susp01);

		// Row stack on the left side of the diagnostics panel.
		auto Row = [&](const FString& S, float& DY_, const FLinearColor& C = TextSecondary())
		{
			DrawText(OutDrawElements, L, AllottedGeometry, S,
				{DX + SpacingMD, DY_}, C, FMicro());
			DY_ += 12.f;
		};

		float Cur = DY + 28.f;
		const float SectionGap = 6.f;

		Row(TEXT("ALS"), Cur, TextMuted());
		Row(FString::Printf(TEXT("  STANCE %s   GAIT %s"),
			*Als.AlsStance.ToString(), *Als.AlsGait.ToString()), Cur);
		Row(FString::Printf(TEXT("  SPD %.1f  MV %d  IN %d"),
			Als.Speed, Als.bMoving ? 1 : 0, Als.bHasInput ? 1 : 0), Cur);
		Row(FString::Printf(TEXT("  LOCMODE %s"),
			*Als.AlsLocomotionMode.ToString()), Cur);
		Cur += SectionGap;

		Row(TEXT("VIS"), Cur, TextMuted());
		Row(FString::Printf(TEXT("  CUR %.2f  LIGHT %.2f  STANCE %.2f"),
			Vis.CurrentVisibility, Vis.LightExposure, Vis.StanceMultiplier), Cur);
		Row(FString::Printf(TEXT("  MOVE %.2f  ACTION %.2f"),
			Vis.MovementMultiplier, Vis.ActionMultiplier), Cur);
		Row(FString::Printf(TEXT("  SCENE  FINAL %.2f  RAW %.2f  N %d"),
			Ls.FinalExposure, Ls.RawMaxExposure, Ls.CachedLightCount), Cur);
		for (const FStealthBodyLightSampleDebug& Pt : Ls.BodySamples)
		{
			Row(FString::Printf(TEXT("    %s  %.2f"),
				*Pt.SampleName, Pt.Exposure), Cur);
		}
		Cur += SectionGap;

		Row(TEXT("NSE"), Cur, TextMuted());
		Row(FString::Printf(TEXT("  R %.0f  EVENTS %d"),
			Snd.Radius, Events.Num()), Cur);
		for (int32 i = 0; i < FMath::Min(3, Events.Num()); ++i)
		{
			Row(FString::Printf(TEXT("  [%d] %s  R %.0f"),
				Events[i].EventId, *Events[i].DebugLabel, Events[i].Radius), Cur);
		}
		Cur += SectionGap;

		Row(TEXT("AI"), Cur, TextMuted());
		if (PrimaryBrain)
		{
			Row(FString::Printf(TEXT("  %s"), *PrimaryBrain->GetDebugBrainLine()), Cur, TextPrimary());
		}
		Row(FString::Printf(TEXT("  ALARM %s   OUTCOME %s"),
			Sim->GetAlarmState().bActive ? TEXT("ON") : TEXT("OFF"),
			*EnumDisplay(Sim->GetMissionOutcome())), Cur);
		if (HP.Owner)
		{
			Row(FString::Printf(TEXT("  HP %.0f/%.0f  %s"),
				HP.CurrentHealth, HP.MaxHealth, *EnumDisplay(HP.VitalState)), Cur);
		}
	}

	return L;
}
