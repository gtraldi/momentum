#include "SurfCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Camera/CameraComponent.h"
#include "FluxMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/TriggerBox.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"
#include "Components/AudioComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"
#include "Styling/CoreStyle.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

namespace SurfHUDStyle
{
	static constexpr float SpawnPanelWidth = 680.0f;
	static constexpr float SpawnPanelHeight = 172.0f;
	static constexpr float VictoryPanelWidth = 760.0f;
	static constexpr float VictoryPanelHeight = 368.0f;
	static constexpr float StatPodWidth = 248.0f;
	static constexpr float StatPodHeight = 78.0f;

	static FSlateFontInfo MakeFont(const ANSICHAR* StyleName, int32 Size, int32 LetterSpacing, int32 OutlineSize, const FLinearColor& OutlineColor)
	{
		FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle(StyleName, Size);
		FontInfo.LetterSpacing = LetterSpacing;
		FontInfo.OutlineSettings.OutlineSize = OutlineSize;
		FontInfo.OutlineSettings.OutlineColor = OutlineColor;
		FontInfo.OutlineSettings.bSeparateFillAlpha = true;
		return FontInfo;
	}

	// Avalia a função de distância com sinal (SDF) para a silhueta temática de cada fase
	static float EvaluateThemedShapeSDF(float X, float Y, float HalfW, float HalfH, int32 ThemeIdx, float ScaleFactor)
	{
		const float PAx = FMath::Abs(X - HalfW);
		const float PAy = FMath::Abs(Y - HalfH);

		if (ThemeIdx == 0)
		{
			// Fase 01 (Costa Tropical): Cápsula costeira / prancha com cantos amplamente arredondados
			const float Radius = FMath::Min(HalfH * 0.42f, 32.0f * ScaleFactor);
			const float Qx = PAx - (HalfW - Radius);
			const float Qy = PAy - (HalfH - Radius);
			const float OutsideDist = FMath::Sqrt(FMath::Square(FMath::Max(Qx, 0.0f)) + FMath::Square(FMath::Max(Qy, 0.0f)));
			const float InsideDist = FMath::Min(FMath::Max(Qx, Qy), 0.0f);
			return OutsideDist + InsideDist - Radius;
		}
		else if (ThemeIdx == 1)
		{
			// Fase 02 (Santuário Kitsune Oriental): Placa tradicional japonesa (Ema / Pergaminho) com cantos côncavos entalhados
			const float BoxDist = FMath::Max(PAx - HalfW, PAy - HalfH);
			const float NotchRadius = 21.0f * ScaleFactor;
			const float CornerDx = PAx - HalfW;
			const float CornerDy = PAy - HalfH;
			const float DistToCornerVertex = FMath::Sqrt(CornerDx * CornerDx + CornerDy * CornerDy);
			// Entalhe circular côncavo nos 4 vértices
			return FMath::Max(BoxDist, NotchRadius - DistToCornerVertex);
		}
		else
		{
			// Fase 03 (Arena Cyber Neon): Moldura HUD Sci-Fi com cortes chanfrados assimétricos de 45 graus
			const float BoxDist = FMath::Max(PAx - HalfW, PAy - HalfH);
			const float RelX = X - HalfW;
			const float RelY = Y - HalfH;
			// Corte maior nos cantos Superior-Esquerdo e Inferior-Direito; corte menor nos opostos
			const bool bMajorDiagonal = ((RelX < 0.0f && RelY < 0.0f) || (RelX > 0.0f && RelY > 0.0f));
			const float CutSize = (bMajorDiagonal ? 28.0f : 14.0f) * ScaleFactor;
			const float ChamferDist = ((PAx + PAy) - (HalfW + HalfH - CutSize)) * 0.70710678f;
			return FMath::Max(BoxDist, ChamferDist);
		}
	}
}

UTexture2D* USurfStageHUDWidget::CreateThemedPanelTexture(int32 Width, int32 Height, int32 ThemeIdx, bool bIsVictory, bool bIsAlert)
{
	UTexture2D* PanelTex = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
	if (!PanelTex || !PanelTex->GetPlatformData() || PanelTex->GetPlatformData()->Mips.Num() == 0)
	{
		return nullptr;
	}

	PanelTex->SRGB = true;
	PanelTex->Filter = TF_Bilinear;
	PanelTex->AddressX = TA_Clamp;
	PanelTex->AddressY = TA_Clamp;

	const float HalfW = Width * 0.5f;
	const float HalfH = Height * 0.5f;
	const float Margin = 3.0f;
	const float EffHalfW = HalfW - Margin;
	const float EffHalfH = HalfH - Margin;
	const float ScaleFactor = bIsVictory ? 1.15f : 0.95f;

	FTexture2DMipMap& Mip = PanelTex->GetPlatformData()->Mips[0];
	void* RawData = Mip.BulkData.Lock(LOCK_READ_WRITE);
	if (RawData)
	{
		FColor* Pixels = static_cast<FColor*>(RawData);

		for (int32 Y = 0; Y < Height; ++Y)
		{
			const float NormY = static_cast<float>(Y) / static_cast<float>(FMath::Max(1, Height - 1));
			for (int32 X = 0; X < Width; ++X)
			{
				const float NormX = static_cast<float>(X) / static_cast<float>(FMath::Max(1, Width - 1));
				const float SDF = SurfHUDStyle::EvaluateThemedShapeSDF(static_cast<float>(X), static_cast<float>(Y), HalfW, HalfH, ThemeIdx, ScaleFactor) + Margin;

				if (SDF > 1.5f)
				{
					Pixels[Y * Width + X] = FColor(0, 0, 0, 0);
					continue;
				}

				// Anti-aliasing suave na borda externa
				const float EdgeAlpha = FMath::Clamp(1.0f - (SDF + 0.5f) / 1.6f, 0.0f, 1.0f);
				const float DistInside = -SDF;

				FLinearColor FinalColor(0.0f, 0.0f, 0.0f, 0.92f);

				if (ThemeIdx == 0)
				{
					// ========================================================
					// FASE 01: COSTA TROPICAL (Pôr do Sol Dourado + Lagoa Turquesa + Ondas)
					// ========================================================
					const FLinearColor TopCol = bIsAlert
						? FLinearColor(0.28f, 0.07f, 0.05f, 0.94f)
						: FLinearColor(0.02f, 0.14f, 0.20f, 0.93f);
					const FLinearColor MidCol = bIsAlert
						? FLinearColor(0.18f, 0.04f, 0.04f, 0.94f)
						: FLinearColor(0.03f, 0.23f, 0.29f, 0.93f);
					const FLinearColor BotCol = bIsAlert
						? FLinearColor(0.32f, 0.10f, 0.05f, 0.95f)
						: FLinearColor(0.05f, 0.36f, 0.40f, 0.94f);

					FLinearColor BgCol = (NormY < 0.5f)
						? FMath::Lerp(TopCol, MidCol, NormY * 2.0f)
						: FMath::Lerp(MidCol, BotCol, (NormY - 0.5f) * 2.0f);

					// Brilho solar dourado no topo central
					const float SunDx = (NormX - 0.5f) * 1.6f;
					const float SunDy = (NormY - 0.08f) * 2.4f;
					const float SunGlow = FMath::Clamp(1.0f - FMath::Sqrt(SunDx * SunDx + SunDy * SunDy), 0.0f, 1.0f);
					const FLinearColor SunCol = bIsAlert
						? FLinearColor(1.0f, 0.38f, 0.15f, 1.0f)
						: FLinearColor(1.0f, 0.76f, 0.28f, 1.0f);
					BgCol = FMath::Lerp(BgCol, SunCol, SunGlow * SunGlow * 0.24f);

					// Padrão de ondas oceânicas suaves na metade inferior do painel
					if (NormY > 0.52f)
					{
						const float Wave1 = FMath::Sin(NormX * 18.85f + NormY * 9.0f) * 0.035f;
						const float Wave2 = FMath::Sin(NormX * 31.41f - NormY * 6.0f + 1.2f) * 0.022f;
						const float WaveLine1 = FMath::Abs((NormY - 0.74f) + Wave1);
						const float WaveLine2 = FMath::Abs((NormY - 0.86f) + Wave2);
						if (WaveLine1 < 0.018f || WaveLine2 < 0.014f)
						{
							const FLinearColor WaveAccent = bIsAlert
								? FLinearColor(1.0f, 0.55f, 0.25f, 1.0f)
								: FLinearColor(0.15f, 0.95f, 0.85f, 1.0f);
							BgCol = FMath::Lerp(BgCol, WaveAccent, 0.16f);
						}
					}

					// Moldura dupla costeira (Ouro Solar -> Turquesa Lagoa)
					const FLinearColor BorderOuter = bIsAlert
						? FLinearColor(1.0f, 0.42f, 0.18f, 1.0f)
						: FMath::Lerp(FLinearColor(1.0f, 0.78f, 0.28f, 1.0f), FLinearColor(0.10f, 0.95f, 0.82f, 1.0f), NormY);
					const FLinearColor BorderInner = bIsAlert
						? FLinearColor(1.0f, 0.72f, 0.32f, 0.65f)
						: FLinearColor(0.45f, 0.98f, 0.90f, 0.55f);

					FinalColor = BgCol;
					if (DistInside < 3.0f)
					{
						FinalColor = BorderOuter;
					}
					else if (DistInside >= 7.0f && DistInside <= 8.4f)
					{
						FinalColor = FMath::Lerp(BgCol, BorderInner, 0.65f);
					}
				}
				else if (ThemeIdx == 1)
				{
					// ========================================================
					// FASE 02: SANTUÁRIO KITSUNE (Laca Urushi Carmesim + Vermelhão Torii + Ouro Imperial)
					// ========================================================
					const FLinearColor TopCol = bIsAlert
						? FLinearColor(0.26f, 0.03f, 0.03f, 0.95f)
						: FLinearColor(0.11f, 0.015f, 0.035f, 0.94f);
					const FLinearColor MidCol = bIsAlert
						? FLinearColor(0.36f, 0.05f, 0.04f, 0.95f)
						: FLinearColor(0.20f, 0.03f, 0.06f, 0.94f);
					const FLinearColor BotCol = bIsAlert
						? FLinearColor(0.22f, 0.02f, 0.03f, 0.95f)
						: FLinearColor(0.13f, 0.02f, 0.04f, 0.95f);

					FLinearColor BgCol = (NormY < 0.55f)
						? FMath::Lerp(TopCol, MidCol, NormY / 0.55f)
						: FMath::Lerp(MidCol, BotCol, (NormY - 0.55f) / 0.45f);

					// Brilho de lanterna imperial dourada/sakura no centro superior
					const float LanternDx = (NormX - 0.5f) * 1.5f;
					const float LanternDy = (NormY - 0.25f) * 1.8f;
					const float LanternGlow = FMath::Clamp(1.0f - FMath::Sqrt(LanternDx * LanternDx + LanternDy * LanternDy), 0.0f, 1.0f);
					BgCol = FMath::Lerp(BgCol, FLinearColor(0.95f, 0.52f, 0.22f, 1.0f), LanternGlow * LanternGlow * 0.16f);

					// Marca d'água tradicional Seigaiha (arcos japoneses de nuvens/ondas)
					const float CellW = 44.0f;
					const float CellH = 22.0f;
					const int32 RowIdx = FMath::FloorToInt(Y / CellH);
					const float OffsetX = (RowIdx % 2 == 0) ? 0.0f : (CellW * 0.5f);
					const float LocalX = FMath::Fmod(X + OffsetX, CellW) - (CellW * 0.5f);
					const float LocalY = FMath::Fmod(static_cast<float>(Y), CellH);
					const float ArcR = FMath::Sqrt(LocalX * LocalX + LocalY * LocalY);
					if (FMath::Abs(ArcR - 20.0f) < 1.1f || FMath::Abs(ArcR - 13.0f) < 0.9f)
					{
						BgCol = FMath::Lerp(BgCol, FLinearColor(1.0f, 0.78f, 0.30f, 1.0f), 0.07f);
					}

					// Moldura dupla sagrada: Vermelhão Torii externo + Ouro Imperial interno
					const FLinearColor ToriiRed(0.96f, 0.22f, 0.14f, 1.0f);
					const FLinearColor ImperialGold(1.0f, 0.82f, 0.28f, 1.0f);

					FinalColor = BgCol;
					if (DistInside < 3.2f)
					{
						FinalColor = bIsAlert ? FLinearColor(1.0f, 0.32f, 0.12f, 1.0f) : ToriiRed;
					}
					else if (DistInside >= 7.5f && DistInside <= 9.3f)
					{
						FinalColor = FMath::Lerp(BgCol, ImperialGold, 0.88f);
					}
				}
				else
				{
					// ========================================================
					// FASE 03: ARENA CYBER NEON (Obsidiana Violeta + Scanlines + Ciano & Magenta Neon)
					// ========================================================
					const FLinearColor TopCol = bIsAlert
						? FLinearColor(0.22f, 0.02f, 0.08f, 0.95f)
						: FLinearColor(0.02f, 0.015f, 0.09f, 0.94f);
					const FLinearColor MidCol = bIsAlert
						? FLinearColor(0.14f, 0.01f, 0.06f, 0.95f)
						: FLinearColor(0.05f, 0.02f, 0.16f, 0.94f);
					const FLinearColor BotCol = bIsAlert
						? FLinearColor(0.25f, 0.03f, 0.10f, 0.95f)
						: FLinearColor(0.02f, 0.10f, 0.18f, 0.95f);

					FLinearColor BgCol = FMath::Lerp(TopCol, BotCol, NormY);
					BgCol = FMath::Lerp(BgCol, MidCol, FMath::Sin(NormX * PI) * 0.45f);

					// Scanlines horizontais holográficas a cada 4 pixels
					if (Y % 4 == 0)
					{
						BgCol = FMath::Lerp(BgCol, FLinearColor(0.0f, 0.95f, 1.0f, 1.0f), 0.045f);
					}

					// Grade tecnológica sutil nas laterais
					if ((X % 32 == 0 || Y % 32 == 0) && (NormX < 0.18f || NormX > 0.82f))
					{
						BgCol = FMath::Lerp(BgCol, FLinearColor(0.0f, 0.90f, 1.0f, 1.0f), 0.05f);
					}

					// Borda Neon Dupla: Ciano Elétrico (Topo/Esquerda) -> Magenta Neon (Base/Direita)
					const float DiagBlend = FMath::Clamp((NormX * 0.55f + NormY * 0.45f), 0.0f, 1.0f);
					const FLinearColor CyanNeon(0.0f, 0.96f, 1.0f, 1.0f);
					const FLinearColor MagentaNeon(1.0f, 0.16f, 0.78f, 1.0f);
					const FLinearColor AlertNeon(1.0f, 0.18f, 0.32f, 1.0f);

					const FLinearColor OuterNeon = bIsAlert
						? FMath::Lerp(AlertNeon, FLinearColor(1.0f, 0.65f, 0.10f, 1.0f), DiagBlend)
						: FMath::Lerp(CyanNeon, MagentaNeon, DiagBlend);

					FinalColor = BgCol;
					if (DistInside < 3.0f)
					{
						FinalColor = OuterNeon;
					}
					else if (DistInside >= 7.0f && DistInside <= 8.5f)
					{
						// Linha interna tracejada de telemetria
						if ((X + Y) % 18 < 12)
						{
							FinalColor = FMath::Lerp(BgCol, CyanNeon, 0.50f);
						}
					}
				}

				FinalColor.A *= EdgeAlpha;
				Pixels[Y * Width + X] = FinalColor.ToFColor(true);
			}
		}

		Mip.BulkData.Unlock();
		PanelTex->UpdateResource();
	}

	return PanelTex;
}

UTexture2D* USurfStageHUDWidget::CreateThemedDividerTexture(int32 Width, int32 Height, int32 ThemeIdx, bool bIsAlert)
{
	UTexture2D* DivTex = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
	if (!DivTex || !DivTex->GetPlatformData() || DivTex->GetPlatformData()->Mips.Num() == 0)
	{
		return nullptr;
	}

	DivTex->SRGB = true;
	DivTex->Filter = TF_Bilinear;
	DivTex->AddressX = TA_Clamp;
	DivTex->AddressY = TA_Clamp;

	const float HalfW = Width * 0.5f;
	const float HalfH = Height * 0.5f;

	FTexture2DMipMap& Mip = DivTex->GetPlatformData()->Mips[0];
	void* RawData = Mip.BulkData.Lock(LOCK_READ_WRITE);
	if (RawData)
	{
		FColor* Pixels = static_cast<FColor*>(RawData);
		for (int32 Y = 0; Y < Height; ++Y)
		{
			for (int32 X = 0; X < Width; ++X)
			{
				const float Dx = FMath::Abs(X - HalfW);
				const float Dy = FMath::Abs(Y - HalfH);
				const float NormDx = Dx / HalfW;
				const float Taper = FMath::Clamp(1.0f - NormDx * NormDx, 0.0f, 1.0f);

				FLinearColor Col(1.0f, 1.0f, 1.0f, 0.0f);
				if (ThemeIdx == 0)
				{
					// Tropical: Linha dourada/turquesa com losango solar no centro
					Col = bIsAlert
						? FLinearColor(1.0f, 0.48f, 0.20f, 1.0f)
						: FMath::Lerp(FLinearColor(1.0f, 0.84f, 0.32f, 1.0f), FLinearColor(0.10f, 0.96f, 0.84f, 1.0f), NormDx);
				}
				else if (ThemeIdx == 1)
				{
					// Kitsune: Ouro Imperial no centro com pontas em Vermelhão Torii
					Col = bIsAlert
						? FLinearColor(1.0f, 0.36f, 0.16f, 1.0f)
						: FMath::Lerp(FLinearColor(1.0f, 0.84f, 0.30f, 1.0f), FLinearColor(0.96f, 0.24f, 0.16f, 1.0f), NormDx);
				}
				else
				{
					// Cyber Neon: Laser Ciano -> Magenta
					Col = bIsAlert
						? FLinearColor(1.0f, 0.22f, 0.38f, 1.0f)
						: FMath::Lerp(FLinearColor(0.0f, 0.96f, 1.0f, 1.0f), FLinearColor(1.0f, 0.18f, 0.80f, 1.0f), NormDx);
				}

				// Emblema losango central + haste afilada
				const float DiamondDist = (Dx * 0.35f + Dy);
				float Alpha = 0.0f;
				if (DiamondDist < HalfH * 0.95f)
				{
					Alpha = FMath::Clamp(1.0f - (DiamondDist / (HalfH * 0.95f)), 0.0f, 1.0f);
				}
				if (Dy < 1.6f)
				{
					Alpha = FMath::Max(Alpha, Taper * (1.0f - Dy / 1.8f));
				}

				Col.A = FMath::Clamp(Alpha, 0.0f, 1.0f);
				Pixels[Y * Width + X] = Col.ToFColor(true);
			}
		}
		Mip.BulkData.Unlock();
		DivTex->UpdateResource();
	}

	return DivTex;
}

UTexture2D* USurfStageHUDWidget::CreateThemedBadgeTexture(int32 Width, int32 Height, int32 ThemeIdx, bool bIsAlert)
{
	UTexture2D* BadgeTex = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
	if (!BadgeTex || !BadgeTex->GetPlatformData() || BadgeTex->GetPlatformData()->Mips.Num() == 0)
	{
		return nullptr;
	}

	BadgeTex->SRGB = true;
	BadgeTex->Filter = TF_Bilinear;
	BadgeTex->AddressX = TA_Clamp;
	BadgeTex->AddressY = TA_Clamp;

	const float HalfW = Width * 0.5f;
	const float HalfH = Height * 0.5f;

	FTexture2DMipMap& Mip = BadgeTex->GetPlatformData()->Mips[0];
	void* RawData = Mip.BulkData.Lock(LOCK_READ_WRITE);
	if (RawData)
	{
		FColor* Pixels = static_cast<FColor*>(RawData);
		for (int32 Y = 0; Y < Height; ++Y)
		{
			for (int32 X = 0; X < Width; ++X)
			{
				const float SDF = SurfHUDStyle::EvaluateThemedShapeSDF(static_cast<float>(X), static_cast<float>(Y), HalfW, HalfH, ThemeIdx, 0.42f) + 1.5f;
				if (SDF > 1.2f)
				{
					Pixels[Y * Width + X] = FColor(0, 0, 0, 0);
					continue;
				}

				const float EdgeAlpha = FMath::Clamp(1.0f - (SDF + 0.4f) / 1.4f, 0.0f, 1.0f);
				const float DistInside = -SDF;

				FLinearColor FillCol(0.01f, 0.04f, 0.08f, 0.78f);
				FLinearColor BorderCol(0.0f, 0.90f, 1.0f, 0.90f);

				if (ThemeIdx == 0)
				{
					FillCol = bIsAlert ? FLinearColor(0.22f, 0.05f, 0.03f, 0.85f) : FLinearColor(0.01f, 0.12f, 0.16f, 0.82f);
					BorderCol = bIsAlert ? FLinearColor(1.0f, 0.48f, 0.20f, 0.95f) : FLinearColor(0.15f, 0.96f, 0.84f, 0.92f);
				}
				else if (ThemeIdx == 1)
				{
					FillCol = bIsAlert ? FLinearColor(0.24f, 0.03f, 0.03f, 0.86f) : FLinearColor(0.09f, 0.01f, 0.025f, 0.85f);
					BorderCol = bIsAlert ? FLinearColor(1.0f, 0.36f, 0.15f, 0.95f) : FLinearColor(1.0f, 0.80f, 0.28f, 0.92f);
				}
				else
				{
					FillCol = bIsAlert ? FLinearColor(0.20f, 0.02f, 0.07f, 0.86f) : FLinearColor(0.02f, 0.01f, 0.08f, 0.84f);
					BorderCol = bIsAlert ? FLinearColor(1.0f, 0.22f, 0.38f, 0.95f) : FLinearColor(0.0f, 0.95f, 1.0f, 0.92f);
				}

				FLinearColor FinalCol = (DistInside < 1.8f) ? BorderCol : FillCol;
				FinalCol.A *= EdgeAlpha;
				Pixels[Y * Width + X] = FinalCol.ToFColor(true);
			}
		}
		Mip.BulkData.Unlock();
		BadgeTex->UpdateResource();
	}

	return BadgeTex;
}

UTexture2D* USurfStageHUDWidget::CreateThemedStatPodTexture(int32 Width, int32 Height, int32 ThemeIdx)
{
	return CreateThemedBadgeTexture(Width, Height, ThemeIdx, false);
}

TSharedRef<SWidget> USurfStageHUDWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("SurfStageHUDWidgetTree"));
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	// ========================================================================
	// 1. BANNER SUPERIOR DE INÍCIO DE FASE / RESPAWN (680 x 172)
	// ========================================================================
	SpawnBannerSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SpawnBannerSizeBox"));
	SpawnBannerSizeBox->SetWidthOverride(SurfHUDStyle::SpawnPanelWidth);
	SpawnBannerSizeBox->SetHeightOverride(SurfHUDStyle::SpawnPanelHeight);
	SpawnBannerSizeBox->SetVisibility(ESlateVisibility::Collapsed);

	if (UCanvasPanelSlot* BannerSlot = RootCanvas->AddChildToCanvas(SpawnBannerSizeBox))
	{
		BannerSlot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 0.0f));
		BannerSlot->SetAlignment(FVector2D(0.5f, 0.0f));
		BannerSlot->SetPosition(FVector2D(0.0f, 40.0f));
		BannerSlot->SetAutoSize(true);
	}

	UOverlay* SpawnOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("SpawnOverlay"));
	SpawnBannerSizeBox->SetContent(SpawnOverlay);

	SpawnBgImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SpawnBgImage"));
	if (UOverlaySlot* BgSlot = SpawnOverlay->AddChildToOverlay(SpawnBgImage))
	{
		BgSlot->SetHorizontalAlignment(HAlign_Fill);
		BgSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UVerticalBox* SpawnVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SpawnVBox"));
	if (UOverlaySlot* ContentSlot = SpawnOverlay->AddChildToOverlay(SpawnVBox))
	{
		ContentSlot->SetHorizontalAlignment(HAlign_Fill);
		ContentSlot->SetVerticalAlignment(VAlign_Center);
		ContentSlot->SetPadding(FMargin(36.0f, 16.0f, 36.0f, 16.0f));
	}

	// Tag / Badge Superior da Fase
	UOverlay* SpawnBadgeOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("SpawnBadgeOverlay"));
	if (UVerticalBoxSlot* BadgeBoxSlot = SpawnVBox->AddChildToVerticalBox(SpawnBadgeOverlay))
	{
		BadgeBoxSlot->SetHorizontalAlignment(HAlign_Center);
		BadgeBoxSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	}

	SpawnBadgeBgImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SpawnBadgeBgImage"));
	SpawnBadgeBgImage->SetDesiredSizeOverride(FVector2D(340.0f, 26.0f));
	if (UOverlaySlot* BadgeBgSlot = SpawnBadgeOverlay->AddChildToOverlay(SpawnBadgeBgImage))
	{
		BadgeBgSlot->SetHorizontalAlignment(HAlign_Fill);
		BadgeBgSlot->SetVerticalAlignment(VAlign_Fill);
	}

	SpawnCodeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SpawnCodeText"));
	SpawnCodeText->SetJustification(ETextJustify::Center);
	if (UOverlaySlot* CodeSlot = SpawnBadgeOverlay->AddChildToOverlay(SpawnCodeText))
	{
		CodeSlot->SetHorizontalAlignment(HAlign_Center);
		CodeSlot->SetVerticalAlignment(VAlign_Center);
		CodeSlot->SetPadding(FMargin(18.0f, 3.0f, 18.0f, 3.0f));
	}

	// Título Principal da Fase
	SpawnTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SpawnTitleText"));
	SpawnTitleText->SetJustification(ETextJustify::Center);
	SpawnTitleText->SetShadowOffset(FVector2D(2.5f, 2.5f));
	SpawnTitleText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.90f));
	if (UVerticalBoxSlot* TitleSlot = SpawnVBox->AddChildToVerticalBox(SpawnTitleText))
	{
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
		TitleSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 6.0f));
	}

	// Divisor Ornamental Temático
	SpawnDividerImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SpawnDividerImage"));
	SpawnDividerImage->SetDesiredSizeOverride(FVector2D(440.0f, 8.0f));
	if (UVerticalBoxSlot* DivSlot = SpawnVBox->AddChildToVerticalBox(SpawnDividerImage))
	{
		DivSlot->SetHorizontalAlignment(HAlign_Center);
		DivSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	}

	// Subtítulo Descritivo
	SpawnSubtitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SpawnSubtitleText"));
	SpawnSubtitleText->SetJustification(ETextJustify::Center);
	SpawnSubtitleText->SetShadowOffset(FVector2D(1.5f, 1.5f));
	SpawnSubtitleText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
	if (UVerticalBoxSlot* SubSlot = SpawnVBox->AddChildToVerticalBox(SpawnSubtitleText))
	{
		SubSlot->SetHorizontalAlignment(HAlign_Center);
	}

	// ========================================================================
	// 2. CARD CENTRAL DE VITÓRIA (760 x 368) COM PODS DE ESTATÍSTICAS
	// ========================================================================
	VictoryCardSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("VictoryCardSizeBox"));
	VictoryCardSizeBox->SetWidthOverride(SurfHUDStyle::VictoryPanelWidth);
	VictoryCardSizeBox->SetHeightOverride(SurfHUDStyle::VictoryPanelHeight);
	VictoryCardSizeBox->SetVisibility(ESlateVisibility::Collapsed);

	if (UCanvasPanelSlot* VictorySlot = RootCanvas->AddChildToCanvas(VictoryCardSizeBox))
	{
		VictorySlot->SetAnchors(FAnchors(0.5f, 0.46f, 0.5f, 0.46f));
		VictorySlot->SetAlignment(FVector2D(0.5f, 0.5f));
		VictorySlot->SetPosition(FVector2D(0.0f, 0.0f));
		VictorySlot->SetAutoSize(true);
	}

	UOverlay* VictoryOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("VictoryOverlay"));
	VictoryCardSizeBox->SetContent(VictoryOverlay);

	VictoryBgImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("VictoryBgImage"));
	if (UOverlaySlot* VBgSlot = VictoryOverlay->AddChildToOverlay(VictoryBgImage))
	{
		VBgSlot->SetHorizontalAlignment(HAlign_Fill);
		VBgSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UVerticalBox* VictoryVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VictoryVBox"));
	if (UOverlaySlot* VContentSlot = VictoryOverlay->AddChildToOverlay(VictoryVBox))
	{
		VContentSlot->SetHorizontalAlignment(HAlign_Fill);
		VContentSlot->SetVerticalAlignment(VAlign_Center);
		VContentSlot->SetPadding(FMargin(48.0f, 24.0f, 48.0f, 24.0f));
	}

	// Badge Superior do Card de Vitória
	UOverlay* VictoryBadgeOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("VictoryBadgeOverlay"));
	if (UVerticalBoxSlot* VBadgeSlot = VictoryVBox->AddChildToVerticalBox(VictoryBadgeOverlay))
	{
		VBadgeSlot->SetHorizontalAlignment(HAlign_Center);
		VBadgeSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}

	VictoryBadgeBgImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("VictoryBadgeBgImage"));
	VictoryBadgeBgImage->SetDesiredSizeOverride(FVector2D(410.0f, 28.0f));
	if (UOverlaySlot* VBadgeBgSlot = VictoryBadgeOverlay->AddChildToOverlay(VictoryBadgeBgImage))
	{
		VBadgeBgSlot->SetHorizontalAlignment(HAlign_Fill);
		VBadgeBgSlot->SetVerticalAlignment(VAlign_Fill);
	}

	VictoryHeaderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("VictoryHeaderText"));
	VictoryHeaderText->SetJustification(ETextJustify::Center);
	if (UOverlaySlot* VHeaderSlot = VictoryBadgeOverlay->AddChildToOverlay(VictoryHeaderText))
	{
		VHeaderSlot->SetHorizontalAlignment(HAlign_Center);
		VHeaderSlot->SetVerticalAlignment(VAlign_Center);
		VHeaderSlot->SetPadding(FMargin(20.0f, 4.0f, 20.0f, 4.0f));
	}

	// Título Principal de Vitória
	VictoryTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("VictoryTitleText"));
	VictoryTitleText->SetJustification(ETextJustify::Center);
	VictoryTitleText->SetShadowOffset(FVector2D(3.0f, 3.0f));
	VictoryTitleText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.92f));
	if (UVerticalBoxSlot* VTitleSlot = VictoryVBox->AddChildToVerticalBox(VictoryTitleText))
	{
		VTitleSlot->SetHorizontalAlignment(HAlign_Center);
		VTitleSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 8.0f));
	}

	// Divisor Ornamental do Card de Vitória
	VictoryDividerImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("VictoryDividerImage"));
	VictoryDividerImage->SetDesiredSizeOverride(FVector2D(520.0f, 10.0f));
	if (UVerticalBoxSlot* VDivSlot = VictoryVBox->AddChildToVerticalBox(VictoryDividerImage))
	{
		VDivSlot->SetHorizontalAlignment(HAlign_Center);
		VDivSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
	}

	// Linha Horizontal com 2 Pods de Estatísticas (TEMPO FINAL | VELOCIDADE)
	UHorizontalBox* StatsHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("StatsHBox"));
	if (UVerticalBoxSlot* StatsRowSlot = VictoryVBox->AddChildToVerticalBox(StatsHBox))
	{
		StatsRowSlot->SetHorizontalAlignment(HAlign_Center);
		StatsRowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
	}

	// Pod 1: Tempo da Fase
	UOverlay* TimePodOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("TimePodOverlay"));
	if (UHorizontalBoxSlot* TimePodSlot = StatsHBox->AddChildToHorizontalBox(TimePodOverlay))
	{
		TimePodSlot->SetPadding(FMargin(0.0f, 0.0f, 12.0f, 0.0f));
	}

	TimePodBgImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("TimePodBgImage"));
	TimePodBgImage->SetDesiredSizeOverride(FVector2D(SurfHUDStyle::StatPodWidth, SurfHUDStyle::StatPodHeight));
	if (UOverlaySlot* TPBgSlot = TimePodOverlay->AddChildToOverlay(TimePodBgImage))
	{
		TPBgSlot->SetHorizontalAlignment(HAlign_Fill);
		TPBgSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UVerticalBox* TimePodVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TimePodVBox"));
	if (UOverlaySlot* TPVBoxSlot = TimePodOverlay->AddChildToOverlay(TimePodVBox))
	{
		TPVBoxSlot->SetHorizontalAlignment(HAlign_Center);
		TPVBoxSlot->SetVerticalAlignment(VAlign_Center);
		TPVBoxSlot->SetPadding(FMargin(16.0f, 8.0f, 16.0f, 8.0f));
	}

	TimeStatLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TimeStatLabelText"));
	TimeStatLabelText->SetText(FText::FromString(TEXT("TEMPO FINAL")));
	TimeStatLabelText->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* TLabelSlot = TimePodVBox->AddChildToVerticalBox(TimeStatLabelText))
	{
		TLabelSlot->SetHorizontalAlignment(HAlign_Center);
		TLabelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 3.0f));
	}

	TimeStatValueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TimeStatValueText"));
	TimeStatValueText->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* TValSlot = TimePodVBox->AddChildToVerticalBox(TimeStatValueText))
	{
		TValSlot->SetHorizontalAlignment(HAlign_Center);
	}

	// Pod 2: Velocidade de Chegada
	UOverlay* SpeedPodOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("SpeedPodOverlay"));
	if (UHorizontalBoxSlot* SpeedPodSlot = StatsHBox->AddChildToHorizontalBox(SpeedPodOverlay))
	{
		SpeedPodSlot->SetPadding(FMargin(12.0f, 0.0f, 0.0f, 0.0f));
	}

	SpeedPodBgImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SpeedPodBgImage"));
	SpeedPodBgImage->SetDesiredSizeOverride(FVector2D(SurfHUDStyle::StatPodWidth, SurfHUDStyle::StatPodHeight));
	if (UOverlaySlot* SPBgSlot = SpeedPodOverlay->AddChildToOverlay(SpeedPodBgImage))
	{
		SPBgSlot->SetHorizontalAlignment(HAlign_Fill);
		SPBgSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UVerticalBox* SpeedPodVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SpeedPodVBox"));
	if (UOverlaySlot* SPVBoxSlot = SpeedPodOverlay->AddChildToOverlay(SpeedPodVBox))
	{
		SPVBoxSlot->SetHorizontalAlignment(HAlign_Center);
		SPVBoxSlot->SetVerticalAlignment(VAlign_Center);
		SPVBoxSlot->SetPadding(FMargin(16.0f, 8.0f, 16.0f, 8.0f));
	}

	SpeedStatLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SpeedStatLabelText"));
	SpeedStatLabelText->SetText(FText::FromString(TEXT("VELOCIDADE FINAL")));
	SpeedStatLabelText->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* SLabelSlot = SpeedPodVBox->AddChildToVerticalBox(SpeedStatLabelText))
	{
		SLabelSlot->SetHorizontalAlignment(HAlign_Center);
		SLabelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 3.0f));
	}

	SpeedStatValueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SpeedStatValueText"));
	SpeedStatValueText->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* SValSlot = SpeedPodVBox->AddChildToVerticalBox(SpeedStatValueText))
	{
		SValSlot->SetHorizontalAlignment(HAlign_Center);
	}

	// Rodapé da Próxima Fase
	VictoryNextStageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("VictoryNextStageText"));
	VictoryNextStageText->SetJustification(ETextJustify::Center);
	VictoryNextStageText->SetShadowOffset(FVector2D(1.5f, 1.5f));
	VictoryNextStageText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
	if (UVerticalBoxSlot* VNextSlot = VictoryVBox->AddChildToVerticalBox(VictoryNextStageText))
	{
		VNextSlot->SetHorizontalAlignment(HAlign_Center);
	}

	// ========================================================================
	// 3. VÉU DE FADE EM TELA CHEIA (TRANSIÇÕES SUAVES)
	// ========================================================================
	FadeVeilImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("FadeVeilImage"));
	FadeVeilImage->SetColorAndOpacity(FLinearColor(0.01f, 0.02f, 0.06f, 0.0f));
	FadeVeilImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* VeilSlot = RootCanvas->AddChildToCanvas(FadeVeilImage))
	{
		VeilSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		VeilSlot->SetOffsets(FMargin(0.0f));
	}

	return Super::RebuildWidget();
}

void USurfStageHUDWidget::ApplyStageTheme(const FString& LevelName)
{
	if (LevelName.Contains(TEXT("Mapa1"), ESearchCase::IgnoreCase))
	{
		CurrentThemeIndex = 0; // Fase 01: Costa Tropical
	}
	else if (LevelName.Contains(TEXT("Kitsune"), ESearchCase::IgnoreCase))
	{
		CurrentThemeIndex = 1; // Fase 02: Santuário Kitsune Oriental
	}
	else
	{
		CurrentThemeIndex = 2; // Fase 03: Arena Cyber Neon
	}

	// Gera as texturas procedurais de alta definição para a fase atual
	CachedSpawnPanelTex = CreateThemedPanelTexture(680, 172, CurrentThemeIndex, false, false);
	CachedSpawnBadgeTex = CreateThemedBadgeTexture(340, 26, CurrentThemeIndex, false);
	CachedSpawnDividerTex = CreateThemedDividerTexture(440, 8, CurrentThemeIndex, false);

	CachedAlertPanelTex = CreateThemedPanelTexture(680, 172, CurrentThemeIndex, false, true);
	CachedAlertBadgeTex = CreateThemedBadgeTexture(340, 26, CurrentThemeIndex, true);
	CachedAlertDividerTex = CreateThemedDividerTexture(440, 8, CurrentThemeIndex, true);

	CachedVictoryPanelTex = CreateThemedPanelTexture(760, 368, CurrentThemeIndex, true, false);
	CachedVictoryBadgeTex = CreateThemedBadgeTexture(410, 28, CurrentThemeIndex, false);
	CachedVictoryDividerTex = CreateThemedDividerTexture(520, 10, CurrentThemeIndex, false);
	CachedStatPodTex = CreateThemedStatPodTexture(248, 78, CurrentThemeIndex);

	if (SpawnBgImage && CachedSpawnPanelTex) SpawnBgImage->SetBrushFromTexture(CachedSpawnPanelTex, false);
	if (SpawnBadgeBgImage && CachedSpawnBadgeTex) SpawnBadgeBgImage->SetBrushFromTexture(CachedSpawnBadgeTex, false);
	if (SpawnDividerImage && CachedSpawnDividerTex) SpawnDividerImage->SetBrushFromTexture(CachedSpawnDividerTex, false);

	if (VictoryBgImage && CachedVictoryPanelTex) VictoryBgImage->SetBrushFromTexture(CachedVictoryPanelTex, false);
	if (VictoryBadgeBgImage && CachedVictoryBadgeTex) VictoryBadgeBgImage->SetBrushFromTexture(CachedVictoryBadgeTex, false);
	if (VictoryDividerImage && CachedVictoryDividerTex) VictoryDividerImage->SetBrushFromTexture(CachedVictoryDividerTex, false);
	if (TimePodBgImage && CachedStatPodTex) TimePodBgImage->SetBrushFromTexture(CachedStatPodTex, false);
	if (SpeedPodBgImage && CachedStatPodTex) SpeedPodBgImage->SetBrushFromTexture(CachedStatPodTex, false);

	// Configuração tipográfica (Fontes, LetterSpacing, Contornos e Paleta) específica de cada Fase
	FLinearColor BadgeTextCol;
	FLinearColor TitleCol;
	FLinearColor VictoryTitleCol;
	FLinearColor SubCol;
	FLinearColor StatLabelCol;
	FLinearColor StatValueCol;
	FLinearColor OutlineCol;

	const ANSICHAR* TitleFontStyle = "Bold";
	const ANSICHAR* SubFontStyle = "Regular";
	const ANSICHAR* StatValueFontStyle = "Bold";
	int32 BadgeTracking = 180;
	int32 TitleTracking = 120;
	int32 SubTracking = 45;

	if (CurrentThemeIndex == 0)
	{
		// Tema 0: Costa Tropical (Tipografia Solar Luminosa + Contorno Azul-Oceano Profundo)
		BadgeTextCol = FLinearColor(0.20f, 0.98f, 0.88f, 1.0f);
		TitleCol = FLinearColor(1.0f, 0.92f, 0.52f, 1.0f);
		VictoryTitleCol = FLinearColor(1.0f, 0.86f, 0.34f, 1.0f);
		SubCol = FLinearColor(0.88f, 0.99f, 0.97f, 1.0f);
		StatLabelCol = FLinearColor(0.25f, 0.96f, 0.86f, 1.0f);
		StatValueCol = FLinearColor(1.0f, 0.96f, 0.78f, 1.0f);
		OutlineCol = FLinearColor(0.01f, 0.08f, 0.12f, 0.96f);
		TitleFontStyle = "Bold";
		SubFontStyle = "Italic";
		StatValueFontStyle = "Bold";
		BadgeTracking = 170;
		TitleTracking = 130;
		SubTracking = 50;
	}
	else if (CurrentThemeIndex == 1)
	{
		// Tema 1: Santuário Kitsune Oriental (Tipografia Cerimonial Espaçada + Marfim Imperial & Sakura + Contorno Laca Carmesim)
		BadgeTextCol = FLinearColor(1.0f, 0.82f, 0.36f, 1.0f);
		TitleCol = FLinearColor(1.0f, 0.94f, 0.76f, 1.0f);
		VictoryTitleCol = FLinearColor(1.0f, 0.84f, 0.32f, 1.0f);
		SubCol = FLinearColor(1.0f, 0.84f, 0.88f, 1.0f);
		StatLabelCol = FLinearColor(1.0f, 0.52f, 0.44f, 1.0f);
		StatValueCol = FLinearColor(1.0f, 0.94f, 0.78f, 1.0f);
		OutlineCol = FLinearColor(0.09f, 0.005f, 0.015f, 0.98f);
		TitleFontStyle = "Bold";
		SubFontStyle = "Italic";
		StatValueFontStyle = "Bold";
		BadgeTracking = 240;
		TitleTracking = 210;
		SubTracking = 75;
	}
	else
	{
		// Tema 2: Arena Cyber Neon (Tipografia Técnica Futurista / Monospace + Ciano Elétrico & Magenta Neon)
		BadgeTextCol = FLinearColor(0.0f, 0.96f, 1.0f, 1.0f);
		TitleCol = FLinearColor(0.95f, 0.99f, 1.0f, 1.0f);
		VictoryTitleCol = FLinearColor(1.0f, 0.32f, 0.88f, 1.0f);
		SubCol = FLinearColor(0.76f, 0.92f, 1.0f, 1.0f);
		StatLabelCol = FLinearColor(0.0f, 0.95f, 1.0f, 1.0f);
		StatValueCol = FLinearColor(0.96f, 0.99f, 1.0f, 1.0f);
		OutlineCol = FLinearColor(0.02f, 0.005f, 0.08f, 0.98f);
		TitleFontStyle = "Bold";
		SubFontStyle = "Mono";
		StatValueFontStyle = "Mono";
		BadgeTracking = 260;
		TitleTracking = 190;
		SubTracking = 80;
	}

	if (SpawnCodeText)
	{
		SpawnCodeText->SetFont(SurfHUDStyle::MakeFont("Bold", 11, BadgeTracking, 1, OutlineCol));
		SpawnCodeText->SetColorAndOpacity(FSlateColor(BadgeTextCol));
	}
	if (SpawnTitleText)
	{
		SpawnTitleText->SetFont(SurfHUDStyle::MakeFont(TitleFontStyle, 30, TitleTracking, 2, OutlineCol));
		SpawnTitleText->SetColorAndOpacity(FSlateColor(TitleCol));
	}
	if (SpawnSubtitleText)
	{
		SpawnSubtitleText->SetFont(SurfHUDStyle::MakeFont(SubFontStyle, 13, SubTracking, 1, OutlineCol));
		SpawnSubtitleText->SetColorAndOpacity(FSlateColor(SubCol));
	}

	if (VictoryHeaderText)
	{
		VictoryHeaderText->SetFont(SurfHUDStyle::MakeFont("Bold", 11, BadgeTracking, 1, OutlineCol));
		VictoryHeaderText->SetColorAndOpacity(FSlateColor(BadgeTextCol));
	}
	if (VictoryTitleText)
	{
		VictoryTitleText->SetFont(SurfHUDStyle::MakeFont(TitleFontStyle, 42, TitleTracking + 30, 3, OutlineCol));
		VictoryTitleText->SetColorAndOpacity(FSlateColor(VictoryTitleCol));
	}
	if (TimeStatLabelText)
	{
		TimeStatLabelText->SetFont(SurfHUDStyle::MakeFont("Bold", 10, 200, 1, OutlineCol));
		TimeStatLabelText->SetColorAndOpacity(FSlateColor(StatLabelCol));
	}
	if (TimeStatValueText)
	{
		TimeStatValueText->SetFont(SurfHUDStyle::MakeFont(StatValueFontStyle, 22, 90, 2, OutlineCol));
		TimeStatValueText->SetColorAndOpacity(FSlateColor(StatValueCol));
	}
	if (SpeedStatLabelText)
	{
		SpeedStatLabelText->SetFont(SurfHUDStyle::MakeFont("Bold", 10, 200, 1, OutlineCol));
		SpeedStatLabelText->SetColorAndOpacity(FSlateColor(StatLabelCol));
	}
	if (SpeedStatValueText)
	{
		SpeedStatValueText->SetFont(SurfHUDStyle::MakeFont(StatValueFontStyle, 22, 90, 2, OutlineCol));
		SpeedStatValueText->SetColorAndOpacity(FSlateColor(StatValueCol));
	}
	if (VictoryNextStageText)
	{
		VictoryNextStageText->SetFont(SurfHUDStyle::MakeFont("Italic", 13, 60, 1, OutlineCol));
		VictoryNextStageText->SetColorAndOpacity(FSlateColor(SubCol));
	}
}

void USurfStageHUDWidget::ShowSpawnBanner(const FText& StageCode, const FText& StageTitle, const FText& StageSubtitle)
{
	if (VictoryCardSizeBox)
	{
		VictoryCardSizeBox->SetVisibility(ESlateVisibility::Collapsed);
	}

	// Restaura a textura normal da fase no banner superior (caso tenha vindo de um Respawn Alert)
	if (SpawnBgImage && CachedSpawnPanelTex) SpawnBgImage->SetBrushFromTexture(CachedSpawnPanelTex, false);
	if (SpawnBadgeBgImage && CachedSpawnBadgeTex) SpawnBadgeBgImage->SetBrushFromTexture(CachedSpawnBadgeTex, false);
	if (SpawnDividerImage && CachedSpawnDividerTex) SpawnDividerImage->SetBrushFromTexture(CachedSpawnDividerTex, false);

	if (SpawnCodeText) SpawnCodeText->SetText(StageCode);
	if (SpawnTitleText) SpawnTitleText->SetText(StageTitle);
	if (SpawnSubtitleText) SpawnSubtitleText->SetText(StageSubtitle);

	if (SpawnBannerSizeBox)
	{
		SpawnBannerSizeBox->SetVisibility(ESlateVisibility::HitTestInvisible);
		SpawnBannerSizeBox->SetRenderOpacity(0.0f);
		SpawnBannerSizeBox->SetRenderTranslation(FVector2D(0.0f, -36.0f));
		SpawnBannerSizeBox->SetRenderScale(FVector2D(0.94f, 0.94f));
	}
}

void USurfStageHUDWidget::UpdateSpawnBannerAnimation(float ElapsedTime, float TotalDuration)
{
	if (!SpawnBannerSizeBox)
	{
		return;
	}

	const float FadeInEnd = 0.42f;
	const float HoldEnd = FMath::Max(FadeInEnd + 0.25f, TotalDuration - 0.55f);

	float Opacity = 1.0f;
	float OffsetY = 0.0f;
	float Scale = 1.0f;

	if (ElapsedTime < FadeInEnd)
	{
		const float Alpha = FMath::Clamp(ElapsedTime / FadeInEnd, 0.0f, 1.0f);
		const float Eased = FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 2.8f);
		Opacity = Eased;
		OffsetY = FMath::Lerp(-36.0f, 0.0f, Eased);
		Scale = FMath::Lerp(0.94f, 1.0f, Eased);
	}
	else if (ElapsedTime > HoldEnd)
	{
		const float OutAlpha = FMath::Clamp((ElapsedTime - HoldEnd) / (TotalDuration - HoldEnd), 0.0f, 1.0f);
		const float EasedOut = FMath::InterpEaseIn(0.0f, 1.0f, OutAlpha, 2.2f);
		Opacity = 1.0f - EasedOut;
		OffsetY = FMath::Lerp(0.0f, -22.0f, EasedOut);
		Scale = FMath::Lerp(1.0f, 0.97f, EasedOut);
	}

	SpawnBannerSizeBox->SetRenderOpacity(Opacity);
	SpawnBannerSizeBox->SetRenderTranslation(FVector2D(0.0f, OffsetY));
	SpawnBannerSizeBox->SetRenderScale(FVector2D(Scale, Scale));

	if (ElapsedTime >= TotalDuration)
	{
		SpawnBannerSizeBox->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void USurfStageHUDWidget::ShowVictoryBanner(const FText& HeaderText, const FText& TitleText, const FText& TimeValueText, const FText& SpeedValueText, const FText& NextStageText)
{
	if (SpawnBannerSizeBox)
	{
		SpawnBannerSizeBox->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (VictoryHeaderText)
	{
		VictoryHeaderText->SetText(HeaderText);
	}
	if (VictoryTitleText)
	{
		VictoryTitleText->SetText(TitleText);
	}
	if (TimeStatValueText)
	{
		TimeStatValueText->SetText(TimeValueText);
	}
	if (SpeedStatValueText)
	{
		SpeedStatValueText->SetText(SpeedValueText);
	}
	if (VictoryNextStageText)
	{
		VictoryNextStageText->SetText(NextStageText);
	}
	if (VictoryCardSizeBox)
	{
		VictoryCardSizeBox->SetVisibility(ESlateVisibility::HitTestInvisible);
		VictoryCardSizeBox->SetRenderOpacity(0.0f);
		VictoryCardSizeBox->SetRenderScale(FVector2D(0.78f, 0.78f));
	}
}

void USurfStageHUDWidget::UpdateVictoryBannerAnimation(float ElapsedTime, float TotalDuration)
{
	if (!VictoryCardSizeBox)
	{
		return;
	}

	const float PopDuration = 0.36f;
	float Opacity = 1.0f;
	float Scale = 1.0f;
	float OffsetY = 0.0f;

	if (ElapsedTime < PopDuration)
	{
		const float Alpha = FMath::Clamp(ElapsedTime / PopDuration, 0.0f, 1.0f);
		const float Eased = FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 3.0f);
		Opacity = Eased;
		// Leve overshoot elástico (0.78 -> 1.035 -> 1.00)
		Scale = (Alpha < 0.75f)
			? FMath::Lerp(0.78f, 1.035f, Alpha / 0.75f)
			: FMath::Lerp(1.035f, 1.0f, (Alpha - 0.75f) / 0.25f);
		OffsetY = FMath::Lerp(26.0f, 0.0f, Eased);
	}
	else if (ElapsedTime > TotalDuration - 0.45f)
	{
		const float OutAlpha = FMath::Clamp((ElapsedTime - (TotalDuration - 0.45f)) / 0.45f, 0.0f, 1.0f);
		Opacity = 1.0f - OutAlpha;
		Scale = FMath::Lerp(1.0f, 1.06f, OutAlpha);
	}

	VictoryCardSizeBox->SetRenderOpacity(Opacity);
	VictoryCardSizeBox->SetRenderScale(FVector2D(Scale, Scale));
	VictoryCardSizeBox->SetRenderTranslation(FVector2D(0.0f, OffsetY));
}

void USurfStageHUDWidget::ShowRespawnBanner()
{
	FText AlertBadge;
	FText AlertTitle;
	FText AlertSub;

	if (CurrentThemeIndex == 0)
	{
		AlertBadge = FText::FromString(TEXT("MARÉ PROFUNDA  •  RETORNO"));
		AlertTitle = FText::FromString(TEXT("REINICIANDO TRAJETO"));
		AlertSub = FText::FromString(TEXT("Retornando à plataforma costeira inicial..."));
	}
	else if (CurrentThemeIndex == 1)
	{
		AlertBadge = FText::FromString(TEXT("ALÉM DAS NUVENS  •  RETORNO"));
		AlertTitle = FText::FromString(TEXT("CAMINHO INTERROMPIDO"));
		AlertSub = FText::FromString(TEXT("Retornando ao pórtico sagrado do santuário..."));
	}
	else
	{
		AlertBadge = FText::FromString(TEXT("FALHA DE TRAJETÓRIA  •  RESET"));
		AlertTitle = FText::FromString(TEXT("SINAL REINICIADO"));
		AlertSub = FText::FromString(TEXT("Recalibrando posição na plataforma de lançamento..."));
	}

	ShowSpawnBanner(AlertBadge, AlertTitle, AlertSub);

	// Aplica a variante de alerta mantendo a silhueta e arte da fase atual
	if (SpawnBgImage && CachedAlertPanelTex) SpawnBgImage->SetBrushFromTexture(CachedAlertPanelTex, false);
	if (SpawnBadgeBgImage && CachedAlertBadgeTex) SpawnBadgeBgImage->SetBrushFromTexture(CachedAlertBadgeTex, false);
	if (SpawnDividerImage && CachedAlertDividerTex) SpawnDividerImage->SetBrushFromTexture(CachedAlertDividerTex, false);

	if (SpawnCodeText)
	{
		SpawnCodeText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.58f, 0.28f, 1.0f)));
	}
	if (SpawnTitleText)
	{
		SpawnTitleText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.88f, 0.72f, 1.0f)));
	}

	if (SpawnBannerSizeBox)
	{
		SpawnBannerSizeBox->SetRenderOpacity(1.0f);
		SpawnBannerSizeBox->SetRenderTranslation(FVector2D(0.0f, 0.0f));
		SpawnBannerSizeBox->SetRenderScale(FVector2D(1.0f, 1.0f));
	}
}

void USurfStageHUDWidget::HideAllBanners()
{
	if (SpawnBannerSizeBox)
	{
		SpawnBannerSizeBox->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (VictoryCardSizeBox)
	{
		VictoryCardSizeBox->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void USurfStageHUDWidget::SetFadeVeilColorAndOpacity(const FLinearColor& InColor)
{
	if (FadeVeilImage)
	{
		FadeVeilImage->SetColorAndOpacity(InColor);
	}
}

// ============================================================================
// ASurfCharacter
// ============================================================================

ASurfCharacter::ASurfCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. Configuração da Cápsula (Dimensões oficiais da Source Engine)
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	CapsuleComponent->InitCapsuleSize(40.0f, 100.0f); // Raio 40cm, Meia-altura 100cm (2 metros no total)
	CapsuleComponent->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	CapsuleComponent->SetGenerateOverlapEvents(true);
	CapsuleComponent->CanCharacterStepUpOn = ECB_No;
	RootComponent = CapsuleComponent;

	// 2. Câmera em Primeira Pessoa (Altura dos olhos a ~160cm do chão)
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(CapsuleComponent);
	CameraComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f)); // Centro da cápsula é 100 + 60 = 160cm
	CameraComponent->bUsePawnControlRotation = true;

	// 3. Componente de Movimento de Surf e Bhop
	MovementComponent = CreateDefaultSubobject<UFluxMovementComponent>(TEXT("FluxMovementComponent"));
	MovementComponent->UpdatedComponent = CapsuleComponent;

	// 4. Componente de Música de Fundo (Lo-Fi por Fase)
	BackgroundMusicComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("BackgroundMusicComponent"));
	BackgroundMusicComponent->SetupAttachment(CapsuleComponent);
	BackgroundMusicComponent->bAutoActivate = false;
	BackgroundMusicComponent->bIsUISound = true;

	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	// 5. Carrega os efeitos sonoros padrão de Spawn, Vitória, Queda, Pulo, Aterrissagem e Músicas Lo-Fi
	static ConstructorHelpers::FObjectFinder<USoundWave> SpawnSoundFinder(TEXT("/Game/Surf/Audio/SW_StageSpawn.SW_StageSpawn"));
	if (SpawnSoundFinder.Succeeded())
	{
		SpawnSound = SpawnSoundFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundWave> VictorySoundFinder(TEXT("/Game/Surf/Audio/SW_StageVictory.SW_StageVictory"));
	if (VictorySoundFinder.Succeeded())
	{
		VictorySound = VictorySoundFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundWave> FallSoundFinder(TEXT("/Game/Surf/Audio/SW_FallRespawn.SW_FallRespawn"));
	if (FallSoundFinder.Succeeded())
	{
		FallRespawnSound = FallSoundFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundWave> JumpSoundFinder(TEXT("/Game/Surf/Audio/SW_SurfJump.SW_SurfJump"));
	if (JumpSoundFinder.Succeeded())
	{
		JumpSound = JumpSoundFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundWave> LandSoundFinder(TEXT("/Game/Surf/Audio/SW_SurfLand.SW_SurfLand"));
	if (LandSoundFinder.Succeeded())
	{
		LandSound = LandSoundFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundWave> TropicalBgmFinder(TEXT("/Game/Surf/Audio/BGM_Lofi_Tropical.BGM_Lofi_Tropical"));
	if (TropicalBgmFinder.Succeeded())
	{
		TropicalBgmFinder.Object->bLooping = true;
		TropicalLofiMusic = TropicalBgmFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundWave> KitsuneBgmFinder(TEXT("/Game/Surf/Audio/BGM_Lofi_Kitsune.BGM_Lofi_Kitsune"));
	if (KitsuneBgmFinder.Succeeded())
	{
		KitsuneBgmFinder.Object->bLooping = true;
		KitsuneLofiMusic = KitsuneBgmFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundWave> CyberBgmFinder(TEXT("/Game/Surf/Audio/BGM_Lofi_Cyber.BGM_Lofi_Cyber"));
	if (CyberBgmFinder.Succeeded())
	{
		CyberBgmFinder.Object->bLooping = true;
		CyberLofiMusic = CyberBgmFinder.Object;
	}
}

void ASurfCharacter::BeginPlay()
{
	Super::BeginPlay();

	InitialSpawnLocation = GetActorLocation();
	InitialSpawnRotation = Controller ? Controller->GetControlRotation() : GetActorRotation();
	if (CameraComponent)
	{
		DefaultCameraFOV = CameraComponent->FieldOfView;
		DefaultCameraZ = CameraComponent->GetRelativeLocation().Z;
	}

	LastRealTickTime = FPlatformTime::Seconds();
	StageRunElapsedTime = 0.0f;

	// Registra o contexto do Enhanced Input se configurado
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}

	EnsureHUDCreated();
	StartStageBackgroundMusic();
	StartSpawnPresentation(true);
}

void ASurfCharacter::StartStageBackgroundMusic()
{
	if (!BackgroundMusicComponent)
	{
		return;
	}

	if (!TropicalLofiMusic)
	{
		TropicalLofiMusic = LoadObject<USoundWave>(nullptr, TEXT("/Game/Surf/Audio/BGM_Lofi_Tropical.BGM_Lofi_Tropical"));
	}
	if (!KitsuneLofiMusic)
	{
		KitsuneLofiMusic = LoadObject<USoundWave>(nullptr, TEXT("/Game/Surf/Audio/BGM_Lofi_Kitsune.BGM_Lofi_Kitsune"));
	}
	if (!CyberLofiMusic)
	{
		CyberLofiMusic = LoadObject<USoundWave>(nullptr, TEXT("/Game/Surf/Audio/BGM_Lofi_Cyber.BGM_Lofi_Cyber"));
	}

	const FString CurrentLevel = UGameplayStatics::GetCurrentLevelName(this, true);
	const FString MapPackageName = GetWorld() ? GetWorld()->GetOutermost()->GetName() : FString();

	USoundBase* SelectedMusic = CyberLofiMusic;
	if (CurrentLevel.Contains(TEXT("Mapa1"), ESearchCase::IgnoreCase) || MapPackageName.Contains(TEXT("Mapa1"), ESearchCase::IgnoreCase))
	{
		SelectedMusic = TropicalLofiMusic;
	}
	else if (CurrentLevel.Contains(TEXT("Kitsune"), ESearchCase::IgnoreCase) || MapPackageName.Contains(TEXT("Kitsune"), ESearchCase::IgnoreCase))
	{
		SelectedMusic = KitsuneLofiMusic;
	}

	if (SelectedMusic)
	{
		if (USoundWave* SoundWave = Cast<USoundWave>(SelectedMusic))
		{
			SoundWave->bLooping = true;
		}
		if (BackgroundMusicComponent->IsPlaying())
		{
			BackgroundMusicComponent->Stop();
		}
		BackgroundMusicComponent->SetSound(SelectedMusic);
		BackgroundMusicComponent->SetVolumeMultiplier(0.13f);
		BackgroundMusicComponent->FadeIn(1.4f, 0.13f, 0.0f);
	}
}

void ASurfCharacter::EnsureHUDCreated()
{
	if (StageHUDWidget)
	{
		return;
	}

	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		StageHUDWidget = CreateWidget<USurfStageHUDWidget>(PC, USurfStageHUDWidget::StaticClass());
		if (StageHUDWidget)
		{
			StageHUDWidget->AddToViewport(100);
		}
	}
}

void ASurfCharacter::GetStageDisplayInfo(const FString& LevelName, FText& OutCode, FText& OutTitle, FText& OutSubtitle) const
{
	if (LevelName.Contains(TEXT("Mapa1"), ESearchCase::IgnoreCase))
	{
		OutCode = FText::FromString(TEXT("FASE 01  •  ILHA PARADISÍACA"));
		OutTitle = FText::FromString(TEXT("COSTA TROPICAL"));
		OutSubtitle = FText::FromString(TEXT("Deslize pelas brisas douradas da lagoa até a plataforma final"));
	}
	else if (LevelName.Contains(TEXT("Kitsune"), ESearchCase::IgnoreCase))
	{
		OutCode = FText::FromString(TEXT("FASE 02  •  CÉU ORIENTAL"));
		OutTitle = FText::FromString(TEXT("SANTUÁRIO KITSUNE"));
		OutSubtitle = FText::FromString(TEXT("Atravesse os arcos sagrados Torii suspensos entre as nuvens"));
	}
	else
	{
		OutCode = FText::FromString(TEXT("FASE 03  •  SETOR FUTURISTA"));
		OutTitle = FText::FromString(TEXT("ARENA CYBER NEON"));
		OutSubtitle = FText::FromString(TEXT("Domine a velocidade máxima nas rampas do circuito de neon"));
	}
}

FText ASurfCharacter::GetNextStageDisplayLabel(const FString& TargetLevelPath) const
{
	if (TargetLevelPath.Contains(TEXT("Kitsune"), ESearchCase::IgnoreCase))
	{
		return FText::FromString(TEXT("Próximo Destino: Fase 02 — Santuário Kitsune..."));
	}
	if (TargetLevelPath.Contains(TEXT("Surf_Demo_Level"), ESearchCase::IgnoreCase))
	{
		return FText::FromString(TEXT("Próximo Destino: Fase 03 — Arena Cyber Neon..."));
	}
	return FText::FromString(TEXT("Campanha Completa! Retornando ao Menu Principal..."));
}

void ASurfCharacter::StartSpawnPresentation(bool bPlayBanner)
{
	EnsureHUDCreated();

	TransitionState = ESurfTransitionState::Spawning;
	TransitionElapsedTime = 0.0f;
	StageRunElapsedTime = 0.0f;

	if (SpawnSound)
	{
		UGameplayStatics::PlaySound2D(this, SpawnSound, 0.42f, 1.0f);
	}

	if (CameraComponent)
	{
		CameraComponent->SetFieldOfView(DefaultCameraFOV + 18.0f);
		CameraComponent->SetRelativeLocation(FVector(0.0f, 0.0f, DefaultCameraZ + 32.0f));
	}

	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 0.75f, FLinearColor(0.01f, 0.02f, 0.06f, 1.0f), false, false);
		}
	}

	if (StageHUDWidget)
	{
		const FString CurrentLevel = UGameplayStatics::GetCurrentLevelName(this, true);
		StageHUDWidget->ApplyStageTheme(CurrentLevel);
		StageHUDWidget->SetFadeVeilColorAndOpacity(FLinearColor(0.01f, 0.02f, 0.06f, 0.95f));
		if (bPlayBanner)
		{
			FText Code, Title, Subtitle;
			GetStageDisplayInfo(CurrentLevel, Code, Title, Subtitle);
			StageHUDWidget->ShowSpawnBanner(Code, Title, Subtitle);
		}
		else
		{
			StageHUDWidget->HideAllBanners();
		}
	}
}

void ASurfCharacter::StartFallRespawnTransition()
{
	if (TransitionState == ESurfTransitionState::FallingRespawn || TransitionState == ESurfTransitionState::Victory)
	{
		return;
	}

	EnsureHUDCreated();
	TransitionState = ESurfTransitionState::FallingRespawn;
	TransitionElapsedTime = 0.0f;

	if (FallRespawnSound)
	{
		UGameplayStatics::PlaySound2D(this, FallRespawnSound, 0.45f, 1.0f);
	}

	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, 0.36f, FLinearColor(0.02f, 0.01f, 0.05f, 1.0f), false, true);
		}
	}

	if (StageHUDWidget)
	{
		const FString CurrentLevel = UGameplayStatics::GetCurrentLevelName(this, true);
		StageHUDWidget->ApplyStageTheme(CurrentLevel);
		StageHUDWidget->ShowRespawnBanner();
	}
}

void ASurfCharacter::StartVictoryTransition(const FString& InTargetLevel)
{
	if (TransitionState == ESurfTransitionState::Victory)
	{
		return;
	}

	EnsureHUDCreated();
	TransitionState = ESurfTransitionState::Victory;
	TransitionElapsedTime = 0.0f;
	PendingNextLevelPath = InTargetLevel;

	if (VictorySound)
	{
		UGameplayStatics::PlaySound2D(this, VictorySound, 0.55f, 1.0f);
	}

	if (BackgroundMusicComponent && BackgroundMusicComponent->IsPlaying())
	{
		BackgroundMusicComponent->FadeOut(1.8f, 0.0f);
	}

	const int32 TotalMinutes = FMath::FloorToInt(StageRunElapsedTime / 60.0f);
	const float RemainderSeconds = FMath::Fmod(StageRunElapsedTime, 60.0f);
	const int32 WholeSeconds = FMath::FloorToInt(RemainderSeconds);
	const int32 Centiseconds = FMath::Clamp(FMath::FloorToInt((RemainderSeconds - WholeSeconds) * 100.0f), 0, 99);
	const int32 ArrivalSpeed = MovementComponent ? FMath::RoundToInt(MovementComponent->Velocity.Size()) : 0;

	const FString CurrentLevel = UGameplayStatics::GetCurrentLevelName(this, true);
	const bool bIsTropical = CurrentLevel.Contains(TEXT("Mapa1"), ESearchCase::IgnoreCase);
	const bool bIsKitsune = CurrentLevel.Contains(TEXT("Kitsune"), ESearchCase::IgnoreCase);

	FText HeaderText;
	FText TitleText;
	FLinearColor FlashColor(0.0f, 0.85f, 1.0f, 0.24f);

	if (bIsTropical)
	{
		HeaderText = FText::FromString(TEXT("FASE 01 CONCLUÍDA  •  COSTA TROPICAL"));
		TitleText = FText::FromString(TEXT("TRAVESSIA CONCLUÍDA!"));
		FlashColor = FLinearColor(1.0f, 0.82f, 0.30f, 0.22f);
	}
	else if (bIsKitsune)
	{
		HeaderText = FText::FromString(TEXT("FASE 02 CONCLUÍDA  •  SANTUÁRIO KITSUNE"));
		TitleText = FText::FromString(TEXT("SANTUÁRIO SUPERADO!"));
		FlashColor = FLinearColor(1.0f, 0.42f, 0.25f, 0.22f);
	}
	else
	{
		HeaderText = FText::FromString(TEXT("CAMPANHA CONCLUÍDA  •  ARENA CYBER NEON"));
		TitleText = FText::FromString(TEXT("CIRCUITO DOMINADO!"));
		FlashColor = FLinearColor(0.0f, 0.92f, 1.0f, 0.24f);
	}

	const FText TimeValueText = FText::FromString(FString::Printf(TEXT("%02d:%02d.%02d"), TotalMinutes, WholeSeconds, Centiseconds));
	const FText SpeedValueText = FText::FromString(FString::Printf(TEXT("%d u/s"), ArrivalSpeed));
	const FText NextStageText = GetNextStageDisplayLabel(PendingNextLevelPath);

	if (StageHUDWidget)
	{
		StageHUDWidget->ApplyStageTheme(CurrentLevel);
		StageHUDWidget->ShowVictoryBanner(HeaderText, TitleText, TimeValueText, SpeedValueText, NextStageText);
		// Leve flash temático no instante em que toca a plataforma final
		StageHUDWidget->SetFadeVeilColorAndOpacity(FlashColor);
	}
}

bool ASurfCharacter::IsStandingOnFlatPlatform() const
{
	if (!MovementComponent || !CapsuleComponent || !GetWorld())
	{
		return false;
	}

	if (!MovementComponent->IsGrounded() || MovementComponent->IsSurfing())
	{
		return false;
	}

	const float HalfHeight = CapsuleComponent->GetScaledCapsuleHalfHeight();
	const FVector Center = CapsuleComponent->GetComponentLocation();
	const FVector TraceStart = Center - FVector(0.0f, 0.0f, HalfHeight - 10.0f);
	const FVector TraceEnd = Center - FVector(0.0f, 0.0f, HalfHeight + 20.0f);

	FHitResult Hit;
	FCollisionQueryParams Params(NAME_None, false, this);
	if (!GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
	{
		return false;
	}

	// Apenas superfícies planas horizontais (plataformas)
	if (Hit.Normal.Z < 0.92f)
	{
		return false;
	}

	if (const AActor* HitActor = Hit.GetActor())
	{
		if (HitActor->GetName().Contains(TEXT("Ramp"), ESearchCase::IgnoreCase))
		{
			return false;
		}
	}

	if (const UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(Hit.GetComponent()))
	{
		if (const UStaticMesh* StaticMesh = MeshComp->GetStaticMesh())
		{
			if (StaticMesh->GetName().Contains(TEXT("Ramp"), ESearchCase::IgnoreCase))
			{
				return false;
			}
		}
	}

	return true;
}

void ASurfCharacter::UpdateMovementAudioAndCameraFeedback(float RealDelta)
{
	if (!MovementComponent)
	{
		return;
	}

	if (SoundCooldownTimer > 0.0f)
	{
		SoundCooldownTimer = FMath::Max(0.0f, SoundCooldownTimer - RealDelta);
	}

	const bool bIsNowOnPlatform = IsStandingOnFlatPlatform();
	const bool bIsNowSurfing = MovementComponent->IsSurfing();

	// Som emitido APENAS na aterrissagem / contato com o chão das plataformas planas (nunca na saída do pulo e nunca nas rampas)
	if (!bWasGroundedLastFrame && bIsNowOnPlatform && AirborneTime > 0.08f)
	{
		if (LandSound && SoundCooldownTimer <= 0.0f)
		{
			const float LandVolume = FMath::Clamp(0.18f + AirborneTime * 0.10f, 0.18f, 0.30f);
			UGameplayStatics::PlaySound2D(this, LandSound, LandVolume, FMath::FRandRange(0.96f, 1.03f));
			SoundCooldownTimer = 0.15f;
		}
		LandingCameraDipOffset = -FMath::Clamp(AirborneTime * 14.0f, 4.0f, 10.0f);
	}

	if (!bIsNowOnPlatform && !bIsNowSurfing)
	{
		AirborneTime += RealDelta;
	}
	else
	{
		AirborneTime = 0.0f;
	}

	bWasGroundedLastFrame = bIsNowOnPlatform;
	bWasSurfingLastFrame = bIsNowSurfing;

	// Recupera suavemente o amortecimento vertical da câmera após pisar na plataforma
	LandingCameraDipOffset = FMath::FInterpTo(LandingCameraDipOffset, 0.0f, RealDelta, 14.0f);
	if (TransitionState == ESurfTransitionState::None && CameraComponent)
	{
		CameraComponent->SetRelativeLocation(FVector(0.0f, 0.0f, DefaultCameraZ + LandingCameraDipOffset));
	}
}

void ASurfCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const double CurrentRealTime = FPlatformTime::Seconds();
	const float RealDelta = (LastRealTickTime > 0.0)
		? FMath::Clamp(static_cast<float>(CurrentRealTime - LastRealTickTime), 0.0f, 0.05f)
		: DeltaTime;
	LastRealTickTime = CurrentRealTime;

	if (TransitionState != ESurfTransitionState::Victory && TransitionState != ESurfTransitionState::FallingRespawn)
	{
		StageRunElapsedTime += RealDelta;
		UpdateMovementAudioAndCameraFeedback(RealDelta);

		// Garante que a música de fundo continue em loop suave durante a partida
		if (BackgroundMusicComponent && BackgroundMusicComponent->GetSound() && !BackgroundMusicComponent->IsPlaying())
		{
			BackgroundMusicComponent->SetVolumeMultiplier(0.13f);
			BackgroundMusicComponent->Play();
		}
	}

	if (TransitionState == ESurfTransitionState::None)
	{
		return;
	}

	TransitionElapsedTime += RealDelta;

	// 1. Animação de Spawn (Fade-In + FOV Settle + Banner da Fase)
	if (TransitionState == ESurfTransitionState::Spawning)
	{
		constexpr float SpawnCameraDuration = 0.72f;
		constexpr float SpawnBannerDuration = 2.35f;

		const float CamAlpha = FMath::Clamp(TransitionElapsedTime / SpawnCameraDuration, 0.0f, 1.0f);
		const float CamEased = FMath::InterpEaseOut(0.0f, 1.0f, CamAlpha, 2.8f);

		if (CameraComponent)
		{
			CameraComponent->SetFieldOfView(FMath::Lerp(DefaultCameraFOV + 18.0f, DefaultCameraFOV, CamEased));
			CameraComponent->SetRelativeLocation(FVector(0.0f, 0.0f, FMath::Lerp(DefaultCameraZ + 32.0f, DefaultCameraZ, CamEased)));
		}

		if (StageHUDWidget)
		{
			const float VeilAlpha = FMath::Clamp(1.0f - (TransitionElapsedTime / 0.55f), 0.0f, 1.0f);
			StageHUDWidget->SetFadeVeilColorAndOpacity(FLinearColor(0.01f, 0.02f, 0.06f, VeilAlpha * VeilAlpha));
			StageHUDWidget->UpdateSpawnBannerAnimation(TransitionElapsedTime, SpawnBannerDuration);
		}

		if (TransitionElapsedTime >= SpawnBannerDuration)
		{
			if (CameraComponent)
			{
				CameraComponent->SetFieldOfView(DefaultCameraFOV);
				CameraComponent->SetRelativeLocation(FVector(0.0f, 0.0f, DefaultCameraZ));
			}
			if (StageHUDWidget)
			{
				StageHUDWidget->SetFadeVeilColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
			}
			TransitionState = ESurfTransitionState::None;
		}
		return;
	}

	// 2. Animação de Queda / Respawn (Fade-Out rápido + Reposicionamento suave no início da fase)
	if (TransitionState == ESurfTransitionState::FallingRespawn)
	{
		constexpr float FallTransitionDuration = 0.40f;
		const float Alpha = FMath::Clamp(TransitionElapsedTime / FallTransitionDuration, 0.0f, 1.0f);
		const float Eased = FMath::InterpEaseIn(0.0f, 1.0f, Alpha, 2.2f);

		if (CameraComponent)
		{
			CameraComponent->SetFieldOfView(FMath::Lerp(DefaultCameraFOV, DefaultCameraFOV + 16.0f, Eased));
		}

		if (StageHUDWidget)
		{
			StageHUDWidget->SetFadeVeilColorAndOpacity(FLinearColor(0.01f, 0.02f, 0.06f, Eased));
		}

		if (TransitionElapsedTime >= FallTransitionDuration)
		{
			if (MovementComponent)
			{
				MovementComponent->Velocity = FVector::ZeroVector;
			}
			SetActorLocationAndRotation(InitialSpawnLocation, InitialSpawnRotation, false, nullptr, ETeleportType::TeleportPhysics);
			if (Controller)
			{
				Controller->SetControlRotation(InitialSpawnRotation);
			}
			StartSpawnPresentation(true);
		}
		return;
	}

	// 3. Animação de Vitória (Desaceleração cinematográfica + Card de Vitória + Fade-Out para a próxima fase)
	if (TransitionState == ESurfTransitionState::Victory)
	{
		constexpr float TotalVictoryDuration = 2.30f;

		// Desacelera suavemente o jogador sobre a plataforma final para evitar que caia da borda durante a celebração
		if (MovementComponent)
		{
			MovementComponent->Velocity = FMath::VInterpTo(MovementComponent->Velocity, FVector::ZeroVector, RealDelta, 4.2f);
		}

		if (CameraComponent)
		{
			if (TransitionElapsedTime < 0.40f)
			{
				const float ZoomAlpha = FMath::Clamp(TransitionElapsedTime / 0.40f, 0.0f, 1.0f);
				const float Eased = FMath::InterpEaseOut(0.0f, 1.0f, ZoomAlpha, 2.5f);
				CameraComponent->SetFieldOfView(FMath::Lerp(DefaultCameraFOV, DefaultCameraFOV - 8.0f, Eased));
				CameraComponent->SetRelativeLocation(FVector(0.0f, 0.0f, FMath::Lerp(DefaultCameraZ, DefaultCameraZ + 18.0f, Eased)));
			}
			else if (TransitionElapsedTime < 1.55f)
			{
				const float GlideAlpha = (TransitionElapsedTime - 0.40f) / (1.55f - 0.40f);
				CameraComponent->SetFieldOfView(FMath::Lerp(DefaultCameraFOV - 8.0f, DefaultCameraFOV - 4.0f, GlideAlpha));
				CameraComponent->SetRelativeLocation(FVector(0.0f, 0.0f, FMath::Lerp(DefaultCameraZ + 18.0f, DefaultCameraZ + 28.0f, GlideAlpha)));
			}
			else
			{
				const float WarpAlpha = FMath::Clamp((TransitionElapsedTime - 1.55f) / (TotalVictoryDuration - 1.55f), 0.0f, 1.0f);
				const float EasedWarp = FMath::InterpEaseIn(0.0f, 1.0f, WarpAlpha, 2.5f);
				CameraComponent->SetFieldOfView(FMath::Lerp(DefaultCameraFOV - 4.0f, DefaultCameraFOV + 24.0f, EasedWarp));
			}
		}

		if (StageHUDWidget)
		{
			StageHUDWidget->UpdateVictoryBannerAnimation(TransitionElapsedTime, TotalVictoryDuration);

			if (TransitionElapsedTime < 0.25f)
			{
				// Dissipa o flash ciano inicial
				const float FlashAlpha = 1.0f - (TransitionElapsedTime / 0.25f);
				StageHUDWidget->SetFadeVeilColorAndOpacity(FLinearColor(0.0f, 0.85f, 1.0f, 0.32f * FlashAlpha));
			}
			else if (TransitionElapsedTime > 1.55f)
			{
				// Fade-out suave para escuro antes de trocar o mapa
				const float FadeOutAlpha = FMath::Clamp((TransitionElapsedTime - 1.55f) / (TotalVictoryDuration - 1.55f), 0.0f, 1.0f);
				const float EasedOut = FMath::InterpEaseInOut(0.0f, 1.0f, FadeOutAlpha, 2.0f);
				StageHUDWidget->SetFadeVeilColorAndOpacity(FLinearColor(0.01f, 0.02f, 0.06f, EasedOut));
			}
			else
			{
				StageHUDWidget->SetFadeVeilColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
			}
		}

		if (TransitionElapsedTime >= TotalVictoryDuration)
		{
			TransitionState = ESurfTransitionState::None;
			if (!PendingNextLevelPath.IsEmpty())
			{
				UGameplayStatics::OpenLevel(this, FName(*PendingNextLevelPath));
			}
		}
	}
}

UPawnMovementComponent* ASurfCharacter::GetMovementComponent() const
{
	return MovementComponent;
}

void ASurfCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Enhanced Input
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASurfCharacter::EnhancedMove);
		}
		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASurfCharacter::EnhancedLook);
		}
		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ASurfCharacter::OnJump);
		}
	}

	// Legacy Axis Bindings para compatibilidade direta com Project Settings
	PlayerInputComponent->BindAxis("MoveForward", this, &ASurfCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASurfCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &ASurfCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ASurfCharacter::LookUp);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ASurfCharacter::OnJump);
	PlayerInputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ASurfCharacter::OnJump);
}

void ASurfCharacter::MoveForward(float Value)
{
	if (TransitionState == ESurfTransitionState::Victory || TransitionState == ESurfTransitionState::FallingRespawn)
	{
		return;
	}
	if (Value != 0.0f)
	{
		// No surf, a movimentação é orientada pelo Yaw do controle
		FRotator Rotation = Controller ? Controller->GetControlRotation() : GetActorRotation();
		FRotator YawRotation(0.0f, Rotation.Yaw, 0.0f);
		FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ASurfCharacter::MoveRight(float Value)
{
	if (TransitionState == ESurfTransitionState::Victory || TransitionState == ESurfTransitionState::FallingRespawn)
	{
		return;
	}
	if (Value != 0.0f)
	{
		FRotator Rotation = Controller ? Controller->GetControlRotation() : GetActorRotation();
		FRotator YawRotation(0.0f, Rotation.Yaw, 0.0f);
		FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, Value);
	}
}

void ASurfCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void ASurfCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void ASurfCharacter::OnJump()
{
	if (TransitionState == ESurfTransitionState::Victory || TransitionState == ESurfTransitionState::FallingRespawn)
	{
		return;
	}
	if (MovementComponent)
	{
		MovementComponent->TryJump();
	}
}

void ASurfCharacter::EnhancedMove(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();
	MoveForward(MovementVector.Y);
	MoveRight(MovementVector.X);
}

void ASurfCharacter::EnhancedLook(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	Turn(LookAxisVector.X);
	LookUp(-LookAxisVector.Y);
}

FString ASurfCharacter::ResolveNextLevelPath(AActor* FinishTriggerActor) const
{
	if (FinishTriggerActor)
	{
		for (const FName& Tag : FinishTriggerActor->Tags)
		{
			const FString TagStr = Tag.ToString();
			if (TagStr.StartsWith(TEXT("NextLevel:")))
			{
				const FString TargetLevel = TagStr.RightChop(10);
				if (!TargetLevel.IsEmpty())
				{
					return TargetLevel;
				}
			}
		}
	}

	const FString CurrentLevel = UGameplayStatics::GetCurrentLevelName(this, true);
	if (CurrentLevel.Equals(TEXT("Mapa1"), ESearchCase::IgnoreCase))
	{
		return TEXT("/Game/Surf/Maps/Surf_Cyber_Kitsune");
	}
	if (CurrentLevel.Equals(TEXT("Surf_Cyber_Kitsune"), ESearchCase::IgnoreCase))
	{
		return TEXT("/Game/Surf/Maps/Surf_Demo_Level");
	}
	return TEXT("/Game/MainMenu/Maps/L_MainMenu");
}

void ASurfCharacter::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (!OtherActor || OtherActor == this)
	{
		return;
	}

	if (TransitionState == ESurfTransitionState::Victory || TransitionState == ESurfTransitionState::FallingRespawn)
	{
		return;
	}

	// 1. Verifica se é o Trigger de Fim de Fase (inicia celebração de Vitória + transição suave para a próxima fase)
	if (OtherActor->ActorHasTag(FName(TEXT("FinishGoal"))))
	{
		const FString NextLevelPath = ResolveNextLevelPath(OtherActor);
		StartVictoryTransition(NextLevelPath);
		return;
	}

	// 2. Verifica se é o Trigger de Queda / Respawn (inicia transição rápida de queda + retorno suave ao início da fase)
	if (OtherActor->ActorHasTag(FName(TEXT("Respawn"))) || OtherActor->IsA<ATriggerBox>())
	{
		StartFallRespawnTransition();
	}
}
