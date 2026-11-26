#include "MainMenuTitleHelper.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/TextBlock.h"
#include "Layout/Clipping.h"
#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "MainMenuTitle"

namespace MainMenuTitleBuilder
{
namespace
{
	// Paleta Split Horizon
	static constexpr const TCHAR* TitleDarkHex = TEXT("231817");
	static constexpr const TCHAR* HorizonCreamHex = TEXT("E8DFD1");

	// Tipografia 104pt Bold com Tracking compacto (40)
	static constexpr int32 TitleFontSize = 104;
	static constexpr int32 TitleLetterSpacing = 40;

	// FWidgetTransform::Shear recebe angulos em graus convertidos via tan(angle)
	static constexpr float SplitShearSlope = -0.18f;

	// Em Roboto-Bold 104pt (138.67px em @ 96 DPI): ascender = 128.65px, capHeight = 98.58px -> centro das maiusculas em Y = 79.35px
	static constexpr float HalfClipHeightPx = 60.0f;
	static constexpr float CapMidYAt104Pt = 79.35f;
	static constexpr float TopTranslateYPx = HalfClipHeightPx - CapMidYAt104Pt;   // -19.35px
	static constexpr float BottomTranslateYPx = -CapMidYAt104Pt;                   // -79.35px
	static constexpr float TopTranslateXPx = 5.0f;
	static constexpr float BottomTranslateXPx = -5.0f;

	// Margem horizontal dentro do SizeBox para evitar corte das pontas inclinadas e deslocadas sob ClipToBoundsAlways
	static constexpr float HorizontalShearGuardPx = 24.0f;

	// Linha de horizonte: espessura 3.0px (2 * 1.5px) estendida 25px alem das extremidades
	static constexpr float HorizonLineHalfThicknessPx = 1.5f;
	static constexpr float HorizonLineHorizontalExtendPx = 25.0f;
	static constexpr float HorizonLineOpacity = 0.95f;
	static constexpr float SubtleShadowOpacity = 0.40f;

	float GetSplitShearDegrees()
	{
		return FMath::RadiansToDegrees(FMath::Atan(SplitShearSlope));
	}

	void AddTitleHalfBox(
		UWidgetTree* WidgetTree,
		UOverlay* ParentOverlay,
		const FName SizeBoxName,
		const FName TextBlockName,
		const FText& TitleText,
		const FSlateFontInfo& TitleFont,
		const FLinearColor& TextColor,
		const FLinearColor& ShadowColor,
		const float ShearDegrees,
		const float TranslateX,
		const float TranslateY,
		const EVerticalAlignment OverlayVAlign,
		const FMargin& OverlayPadding)
	{
		USizeBox* HalfBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), SizeBoxName);
		HalfBox->SetHeightOverride(HalfClipHeightPx);
		HalfBox->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		HalfBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UTextBlock* HalfText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TextBlockName);
		HalfText->SetText(TitleText);
		HalfText->SetFont(TitleFont);
		HalfText->SetColorAndOpacity(FSlateColor(TextColor));
		HalfText->SetShadowOffset(FVector2D(1.5f, 1.5f));
		HalfText->SetShadowColorAndOpacity(ShadowColor);
		HalfText->SetJustification(ETextJustify::Center);
		HalfText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		HalfText->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		HalfText->SetRenderShear(FVector2D(ShearDegrees, 0.0f));
		HalfText->SetRenderTranslation(FVector2D(TranslateX, TranslateY));

		if (USizeBoxSlot* TextSlot = Cast<USizeBoxSlot>(HalfBox->AddChild(HalfText)))
		{
			TextSlot->SetHorizontalAlignment(HAlign_Center);
			// VAlign_Top preserva o DesiredSize vertical inteiro do glyph box para alinhamento exato do corte
			TextSlot->SetVerticalAlignment(VAlign_Top);
			TextSlot->SetPadding(FMargin(HorizontalShearGuardPx, 0.0f, HorizontalShearGuardPx, 0.0f));
		}

		if (UOverlaySlot* HalfSlot = ParentOverlay->AddChildToOverlay(HalfBox))
		{
			HalfSlot->SetHorizontalAlignment(HAlign_Center);
			HalfSlot->SetVerticalAlignment(OverlayVAlign);
			HalfSlot->SetPadding(OverlayPadding);
		}
	}
}

FLinearColor HexToLinear(const TCHAR* HexString, const float Alpha)
{
	FLinearColor Color = FLinearColor::FromSRGBColor(FColor::FromHex(HexString));
	Color.A = Alpha;
	return Color;
}

UOverlay* BuildSplitHorizonTitle(UWidgetTree* WidgetTree)
{
	if (!WidgetTree)
	{
		return nullptr;
	}

	UOverlay* TitleOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Overlay_TitleSplitHorizon"));
	TitleOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	const FText TitleText = LOCTEXT("Title_Momentum", "MOMENTUM");
	FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", TitleFontSize);
	TitleFont.LetterSpacing = TitleLetterSpacing;

	const FLinearColor TextColor = HexToLinear(TitleDarkHex, 1.0f);
	const FLinearColor ShadowColor = HexToLinear(HorizonCreamHex, SubtleShadowOpacity);
	const FLinearColor LineColor = HexToLinear(HorizonCreamHex, HorizonLineOpacity);
	const float ShearDegrees = GetSplitShearDegrees();

	// 1. Metade superior do texto (ocupa [0, HalfClipHeightPx] via padding inferior igual a HalfClipHeightPx)
	AddTitleHalfBox(
		WidgetTree,
		TitleOverlay,
		TEXT("SizeBox_TopHalf"),
		TEXT("TextBlock_Top"),
		TitleText,
		TitleFont,
		TextColor,
		ShadowColor,
		ShearDegrees,
		TopTranslateXPx,
		TopTranslateYPx,
		VAlign_Top,
		FMargin(0.0f, 0.0f, 0.0f, HalfClipHeightPx));

	// 2. Metade inferior do texto (ocupa [HalfClipHeightPx, 2 * HalfClipHeightPx] via padding superior)
	AddTitleHalfBox(
		WidgetTree,
		TitleOverlay,
		TEXT("SizeBox_BottomHalf"),
		TEXT("TextBlock_Bottom"),
		TitleText,
		TitleFont,
		TextColor,
		ShadowColor,
		ShearDegrees,
		BottomTranslateXPx,
		BottomTranslateYPx,
		VAlign_Bottom,
		FMargin(0.0f, HalfClipHeightPx, 0.0f, 0.0f));

	// 3. Linha de horizonte creme atravessando o centro entre as duas metades com pontas chanfradas
	UBorder* HorizonLine = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Border_HorizonLine"));
	FSlateBrush LineBrush;
	LineBrush.DrawAs = ESlateBrushDrawType::Image;
	LineBrush.TintColor = FSlateColor(FLinearColor::White);
	HorizonLine->SetBrush(LineBrush);
	HorizonLine->SetBrushColor(LineColor);
	HorizonLine->SetPadding(FMargin(0.0f, HorizonLineHalfThicknessPx, 0.0f, HorizonLineHalfThicknessPx));
	HorizonLine->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	HorizonLine->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	HorizonLine->SetRenderShear(FVector2D(ShearDegrees, 0.0f));

	if (UOverlaySlot* LineSlot = TitleOverlay->AddChildToOverlay(HorizonLine))
	{
		LineSlot->SetHorizontalAlignment(HAlign_Fill);
		LineSlot->SetVerticalAlignment(VAlign_Center);
		LineSlot->SetPadding(FMargin(-HorizonLineHorizontalExtendPx, 0.0f, -HorizonLineHorizontalExtendPx, 0.0f));
	}

	return TitleOverlay;
}
}

#if WITH_DEV_AUTOMATION_TESTS

#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Misc/AutomationTest.h"
#include "Widgets/SWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSplitHorizonTitleAutomationTest,
	"SurfGame.MainMenu.SplitHorizonTitle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSplitHorizonTitleAutomationTest::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = nullptr;
	if (GEngine)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World())
			{
				TestWorld = Context.World();
				break;
			}
		}
	}
	if (!TestNotNull(TEXT("World for SplitHorizon UMG test"), TestWorld))
	{
		return false;
	}

	const TCHAR* WidgetAssetPaths[] = {
		TEXT("/Game/MainMenu/WBP_MainMenu.WBP_MainMenu_C"),
		TEXT("/Game/MainMenu/WBP_PauseMenu.WBP_PauseMenu_C")
	};

	const double ExpectedShearDeg = FMath::RadiansToDegrees(FMath::Atan(-0.18));
	const FLinearColor ExpectedDarkBrown = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("231817")));
	const FLinearColor ExpectedCreamLine = []()
	{
		FLinearColor C = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("E8DFD1")));
		C.A = 0.95f;
		return C;
	}();

	for (const TCHAR* AssetPath : WidgetAssetPaths)
	{
		UClass* WidgetClass = LoadClass<UUserWidget>(nullptr, AssetPath);
		if (!TestNotNull(*FString::Printf(TEXT("Load %s"), AssetPath), WidgetClass))
		{
			return false;
		}

		UUserWidget* Widget = CreateWidget<UUserWidget>(TestWorld, WidgetClass);
		if (!TestNotNull(*FString::Printf(TEXT("CreateWidget %s"), AssetPath), Widget))
		{
			return false;
		}

		TSharedRef<SWidget> SlateWidget = Widget->TakeWidget();
		SlateWidget->SlatePrepass(1.0f);

		UWidgetTree* Tree = Widget->WidgetTree;
		if (!TestNotNull(TEXT("WidgetTree"), Tree))
		{
			return false;
		}

		UOverlay* Overlay = Cast<UOverlay>(Tree->FindWidget(TEXT("Overlay_TitleSplitHorizon")));
		USizeBox* TopBox = Cast<USizeBox>(Tree->FindWidget(TEXT("SizeBox_TopHalf")));
		USizeBox* BottomBox = Cast<USizeBox>(Tree->FindWidget(TEXT("SizeBox_BottomHalf")));
		UTextBlock* TopText = Cast<UTextBlock>(Tree->FindWidget(TEXT("TextBlock_Top")));
		UTextBlock* BottomText = Cast<UTextBlock>(Tree->FindWidget(TEXT("TextBlock_Bottom")));
		UBorder* HorizonLine = Cast<UBorder>(Tree->FindWidget(TEXT("Border_HorizonLine")));

		TestNotNull(TEXT("Overlay_TitleSplitHorizon exists"), Overlay);
		TestNotNull(TEXT("SizeBox_TopHalf exists"), TopBox);
		TestNotNull(TEXT("SizeBox_BottomHalf exists"), BottomBox);
		TestNotNull(TEXT("TextBlock_Top exists"), TopText);
		TestNotNull(TEXT("TextBlock_Bottom exists"), BottomText);
		TestNotNull(TEXT("Border_HorizonLine exists"), HorizonLine);

		// Verifica ausencia completa do botao Discord e do TextBlock antigo
		TestNull(TEXT("DiscordButton absent"), Tree->FindWidget(TEXT("DiscordButton")));
		TestNull(TEXT("DiscordBtnSizeBox absent"), Tree->FindWidget(TEXT("DiscordBtnSizeBox")));
		TestNull(TEXT("DiscordIconImage absent"), Tree->FindWidget(TEXT("DiscordIconImage")));
		TestNull(TEXT("Old TitleTextBlock absent"), Tree->FindWidget(TEXT("TitleTextBlock")));

		// Verifica presenca da nova linha de Qualidade Grafica (Baixa, Media, Alta, Epica) no submenu de Configuracoes
		UWidget* QualityRow = Tree->FindWidget(TEXT("GraphicsQualityRow"));
		if (!QualityRow)
		{
			QualityRow = Tree->FindWidget(TEXT("PauseGraphicsQualityRow"));
		}
		TestNotNull(TEXT("GraphicsQualityRow exists in Settings panel"), QualityRow);

		if (Overlay && TopBox && BottomBox && TopText && BottomText && HorizonLine)
		{
			TestEqual(TEXT("TopBox Clipping"), TopBox->GetClipping(), EWidgetClipping::ClipToBoundsAlways);
			TestEqual(TEXT("BottomBox Clipping"), BottomBox->GetClipping(), EWidgetClipping::ClipToBoundsAlways);

			TestTrue(TEXT("TopBox DesiredHeight"), FMath::IsNearlyEqual(TopBox->GetDesiredSize().Y, 60.0, 0.05));
			TestTrue(TEXT("BottomBox DesiredHeight"), FMath::IsNearlyEqual(BottomBox->GetDesiredSize().Y, 60.0, 0.05));
			TestTrue(TEXT("Overlay DesiredHeight"), FMath::IsNearlyEqual(Overlay->GetDesiredSize().Y, 120.0, 0.05));
			TestTrue(TEXT("HorizonLine DesiredHeight"), FMath::IsNearlyEqual(HorizonLine->GetDesiredSize().Y, 3.0, 0.05));

			TestEqual(TEXT("TopText String"), TopText->GetText().ToString(), FString(TEXT("MOMENTUM")));
			TestEqual(TEXT("BottomText String"), BottomText->GetText().ToString(), FString(TEXT("MOMENTUM")));
			TestEqual(TEXT("TopText FontSize"), TopText->GetFont().Size, 104.0f);
			TestEqual(TEXT("TopText LetterSpacing"), TopText->GetFont().LetterSpacing, 40);
			TestEqual(TEXT("TopText Color"), TopText->GetColorAndOpacity().GetSpecifiedColor(), ExpectedDarkBrown);
			TestEqual(TEXT("HorizonLine Color"), HorizonLine->GetBrushColor(), ExpectedCreamLine);

			TestTrue(TEXT("TopText ShearX"), FMath::IsNearlyEqual(TopText->GetRenderTransform().Shear.X, ExpectedShearDeg, 0.01));
			TestTrue(TEXT("BottomText ShearX"), FMath::IsNearlyEqual(BottomText->GetRenderTransform().Shear.X, ExpectedShearDeg, 0.01));
			TestTrue(TEXT("HorizonLine ShearX"), FMath::IsNearlyEqual(HorizonLine->GetRenderTransform().Shear.X, ExpectedShearDeg, 0.01));
			TestTrue(TEXT("TopText TranslationX"), FMath::IsNearlyEqual(TopText->GetRenderTransform().Translation.X, 5.0, 0.01));
			TestTrue(TEXT("BottomText TranslationX"), FMath::IsNearlyEqual(BottomText->GetRenderTransform().Translation.X, -5.0, 0.01));
		}
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

#undef LOCTEXT_NAMESPACE
