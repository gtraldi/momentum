#include "PauseMenuWidget.h"
#include "MainMenuTitleHelper.h"
#include "MainMenuSaveGame.h"
#include "MainMenuPlayerController.h"
#include "SurfGameInstance.h"
#include "Framework/Application/SlateApplication.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/BackgroundBlur.h"
#include "Components/WidgetSwitcher.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/TextBlock.h"
#include "Components/Overlay.h"
#include "Components/Slider.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Styling/CoreStyle.h"
#include "InputCoreTypes.h"

#define LOCTEXT_NAMESPACE "PauseMenu"

namespace PauseMenuStyle
{
	static FLinearColor HexToLinear(const TCHAR* HexString, float Alpha = 1.0f)
	{
		FLinearColor Color = FLinearColor::FromSRGBColor(FColor::FromHex(HexString));
		Color.A = Alpha;
		return Color;
	}

	// Mesma paleta Split Horizon e identidade visual do WBP_MainMenu
	static const FLinearColor TitleDarkBrownColor = HexToLinear(TEXT("231817"), 1.0f);
	static const FLinearColor ButtonNormalBgColor = HexToLinear(TEXT("2E1E1C"), 0.95f);
	static const FLinearColor ButtonHoverBgColor = HexToLinear(TEXT("523632"), 0.98f);
	static const FLinearColor ButtonPressedBgColor = HexToLinear(TEXT("1A1110"), 1.0f);
	static const FLinearColor ButtonSubtleBorderColor = HexToLinear(TEXT("3A2624"), 0.85f);

	static const FLinearColor CreamTextColor = HexToLinear(TEXT("F3E3D3"), 1.0f);
	static const FLinearColor PeachAccentColor = HexToLinear(TEXT("E2BA9D"), 1.0f);
	static const FLinearColor WarmSecondaryTextColor = HexToLinear(TEXT("C89578"), 1.0f);
	static const FLinearColor SubmenuCardBgColor = HexToLinear(TEXT("2A1719"), 0.94f);

	static constexpr float ButtonShearSlopeX = -0.22f;
	static constexpr float ButtonShearAngleDeg = -12.407418f;
	static constexpr float ButtonHoverTranslationX = 8.0f;
	static constexpr float ButtonHoverInterpSpeed = 18.0f;
	static constexpr float PauseBackgroundBlurStrength = 25.0f;

	static constexpr float LeftColumnOffsetX = 100.0f;
	static constexpr float MainButtonWidth = 310.0f;
	static constexpr float SubmenuPanelWidth = 420.0f;
	static constexpr float ButtonMinHeight = 50.0f;
	static constexpr float SmallButtonMinWidth = 48.0f;
	static constexpr float ButtonCornerRadius = 2.0f;
	static constexpr float ButtonBorderThickness = 1.0f;
	static constexpr float ButtonVerticalPadding = 10.0f;

	static constexpr int32 SubtitleFontSize = 18;
	static constexpr int32 SubtitleLetterSpacing = 320;
	static constexpr int32 PanelHeaderFontSize = 22;
	static constexpr int32 ButtonFontSize = 18;
	static constexpr int32 ButtonLetterSpacing = 50;
	static constexpr int32 LabelFontSize = 14;
	static constexpr int32 StatusFontSize = 13;
}

UPauseMenuWidget::UPauseMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);

	AvailableResolutions = {
		FIntPoint(1280, 720),
		FIntPoint(1600, 900),
		FIntPoint(1920, 1080),
		FIntPoint(2560, 1080),
		FIntPoint(2560, 1440),
		FIntPoint(3440, 1440),
		FIntPoint(3840, 2160)
	};
}

TSharedRef<SWidget> UPauseMenuWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("PauseMenuWidgetTree"));
	}

	AllStyledButtons.Empty();

	// 1. Raiz (Canvas Panel) transparente
	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	RootCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = RootCanvas;

	// 2. BackgroundBlur_Pause (Z-Order: 0): Full Fill (0,0 a 1,1), Blur Strength = 25.0
	BackgroundBlur_Pause = WidgetTree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass(), TEXT("BackgroundBlur_Pause"));
	BackgroundBlur_Pause->SetBlurStrength(PauseMenuStyle::PauseBackgroundBlurStrength);
	BackgroundBlur_Pause->SetApplyAlphaToBlur(true);
	BackgroundBlur_Pause->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (UCanvasPanelSlot* BlurSlot = RootCanvas->AddChildToCanvas(BackgroundBlur_Pause))
	{
		BlurSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		BlurSlot->SetOffsets(FMargin(0.0f, 0.0f, 0.0f, 0.0f));
		BlurSlot->SetZOrder(0);
	}

	// 3. Container_UI (Canvas Panel - Z-Order: 1)
	Container_UI = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Container_UI"));
	Container_UI->SetRenderOpacity(1.0f);
	Container_UI->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (UCanvasPanelSlot* ContainerSlot = RootCanvas->AddChildToCanvas(Container_UI))
	{
		ContainerSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		ContainerSlot->SetOffsets(FMargin(0.0f, 0.0f, 0.0f, 0.0f));
		ContainerSlot->SetZOrder(1);
	}

	// 4. Lado Direito / Centro-Direito: Logo "MOMENTUM" + Subtitulo "JOGO PAUSADO"
	UVerticalBox* RightTitleBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RightTitleBox"));
	RightTitleBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (UCanvasPanelSlot* TitleBoxSlot = Container_UI->AddChildToCanvas(RightTitleBox))
	{
		TitleBoxSlot->SetAnchors(FAnchors(0.64f, 0.47f, 0.64f, 0.47f));
		TitleBoxSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		TitleBoxSlot->SetPosition(FVector2D(0.0f, 0.0f));
		TitleBoxSlot->SetAutoSize(true);
		TitleBoxSlot->SetZOrder(1);
	}

	Overlay_TitleSplitHorizon = MainMenuTitleBuilder::BuildSplitHorizonTitle(WidgetTree);
	if (Overlay_TitleSplitHorizon)
	{
		if (UVerticalBoxSlot* MainTitleSlot = RightTitleBox->AddChildToVerticalBox(Overlay_TitleSplitHorizon))
		{
			MainTitleSlot->SetHorizontalAlignment(HAlign_Center);
		}
	}

	SubtitleTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SubtitleTextBlock"));
	SubtitleTextBlock->SetText(LOCTEXT("PauseSubtitle_Paused", "JOGO PAUSADO"));
	FSlateFontInfo SubFont = FCoreStyle::GetDefaultFontStyle("Bold", PauseMenuStyle::SubtitleFontSize);
	SubFont.LetterSpacing = PauseMenuStyle::SubtitleLetterSpacing;
	SubtitleTextBlock->SetFont(SubFont);
	SubtitleTextBlock->SetColorAndOpacity(FSlateColor(PauseMenuStyle::TitleDarkBrownColor));
	SubtitleTextBlock->SetShadowOffset(FVector2D(1.5f, 1.5f));
	SubtitleTextBlock->SetShadowColorAndOpacity(PauseMenuStyle::HexToLinear(TEXT("E8DFD1"), 0.40f));
	SubtitleTextBlock->SetJustification(ETextJustify::Center);

	if (UVerticalBoxSlot* SubTitleSlot = RightTitleBox->AddChildToVerticalBox(SubtitleTextBlock))
	{
		SubTitleSlot->SetHorizontalAlignment(HAlign_Center);
		SubTitleSlot->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 0.0f));
	}

	// 5. Lado Esquerdo: Vertical Box ancorada em Left-Center (Anchor X: 0.0, Y: 0.5, Margin Left: 100px)
	UVerticalBox* LeftColumnLayout = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LeftColumnLayout"));
	if (UCanvasPanelSlot* LeftSlot = Container_UI->AddChildToCanvas(LeftColumnLayout))
	{
		LeftSlot->SetAnchors(FAnchors(0.0f, 0.5f, 0.0f, 0.5f));
		LeftSlot->SetAlignment(FVector2D(0.0f, 0.5f));
		LeftSlot->SetPosition(FVector2D(PauseMenuStyle::LeftColumnOffsetX, 0.0f));
		LeftSlot->SetAutoSize(true);
		LeftSlot->SetZOrder(2);
	}

	ContentSwitcher = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("ContentSwitcher"));
	if (UVerticalBoxSlot* SwitcherSlot = LeftColumnLayout->AddChildToVerticalBox(ContentSwitcher))
	{
		SwitcherSlot->SetHorizontalAlignment(HAlign_Left);
		SwitcherSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	StatusMessageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusMessageText"));
	StatusMessageText->SetText(FText::GetEmpty());
	StatusMessageText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", PauseMenuStyle::StatusFontSize));
	StatusMessageText->SetColorAndOpacity(FSlateColor(PauseMenuStyle::CreamTextColor));
	StatusMessageText->SetShadowOffset(FVector2D(1.0f, 1.0f));
	StatusMessageText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
	StatusMessageText->SetJustification(ETextJustify::Left);
	StatusMessageText->SetAutoWrapText(true);
	StatusMessageText->SetVisibility(ESlateVisibility::Collapsed);

	if (UVerticalBoxSlot* StatusSlot = LeftColumnLayout->AddChildToVerticalBox(StatusMessageText))
	{
		StatusSlot->SetHorizontalAlignment(HAlign_Left);
		StatusSlot->SetPadding(FMargin(4.0f, 6.0f, 0.0f, 0.0f));
	}

	BuildPauseMainPanel();
	BuildSettingsPanel();

	ContentSwitcher->SetActiveWidgetIndex(PanelIndex_Main);

	return Super::RebuildWidget();
}

void UPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ResumeButton)
	{
		ResumeButton->OnClicked.RemoveAll(this);
		ResumeButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnResumeClicked);
	}
	if (SettingsButton)
	{
		SettingsButton->OnClicked.RemoveAll(this);
		SettingsButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnSettingsClicked);
	}
	if (MainMenuButton)
	{
		MainMenuButton->OnClicked.RemoveAll(this);
		MainMenuButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnMainMenuClicked);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.RemoveAll(this);
		QuitButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnQuitClicked);
	}

	if (VolumeSlider)
	{
		VolumeSlider->OnValueChanged.RemoveAll(this);
		VolumeSlider->OnValueChanged.AddDynamic(this, &UPauseMenuWidget::OnVolumeChanged);
	}
	if (SensitivitySlider)
	{
		SensitivitySlider->OnValueChanged.RemoveAll(this);
		SensitivitySlider->OnValueChanged.AddDynamic(this, &UPauseMenuWidget::OnSensitivityChanged);
	}
	if (FOVSlider)
	{
		FOVSlider->OnValueChanged.RemoveAll(this);
		FOVSlider->OnValueChanged.AddDynamic(this, &UPauseMenuWidget::OnFOVChanged);
	}
	if (WindowModePrevButton)
	{
		WindowModePrevButton->OnClicked.RemoveAll(this);
		WindowModePrevButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnWindowModePrevClicked);
	}
	if (WindowModeNextButton)
	{
		WindowModeNextButton->OnClicked.RemoveAll(this);
		WindowModeNextButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnWindowModeNextClicked);
	}
	if (ResolutionPrevButton)
	{
		ResolutionPrevButton->OnClicked.RemoveAll(this);
		ResolutionPrevButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnResolutionPrevClicked);
	}
	if (ResolutionNextButton)
	{
		ResolutionNextButton->OnClicked.RemoveAll(this);
		ResolutionNextButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnResolutionNextClicked);
	}
	if (GraphicsQualityPrevButton)
	{
		GraphicsQualityPrevButton->OnClicked.RemoveAll(this);
		GraphicsQualityPrevButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnGraphicsQualityPrevClicked);
	}
	if (GraphicsQualityNextButton)
	{
		GraphicsQualityNextButton->OnClicked.RemoveAll(this);
		GraphicsQualityNextButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnGraphicsQualityNextClicked);
	}
	if (SettingsApplyButton)
	{
		SettingsApplyButton->OnClicked.RemoveAll(this);
		SettingsApplyButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnSettingsApplyClicked);
	}
	if (SettingsBackButton)
	{
		SettingsBackButton->OnClicked.RemoveAll(this);
		SettingsBackButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnBackToPauseMainClicked);
	}

	LoadSettingsToUI();
	FocusInitialButton();
}

void UPauseMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Animacao de Hover (Translation X: +8px) mesmo com o jogo pausado
	for (UButton* Btn : AllStyledButtons)
	{
		if (!Btn)
		{
			continue;
		}

		const bool bHighlighted = Btn->IsHovered() || Btn->HasKeyboardFocus();
		const float TargetX = bHighlighted ? PauseMenuStyle::ButtonHoverTranslationX : 0.0f;
		const float CurrentX = Btn->GetRenderTransform().Translation.X;
		const float NewX = FMath::FInterpTo(CurrentX, TargetX, InDeltaTime, PauseMenuStyle::ButtonHoverInterpSpeed);
		if (!FMath::IsNearlyEqual(CurrentX, NewX, 0.05f))
		{
			Btn->SetRenderTranslation(FVector2D(NewX, 0.0f));
		}
	}
}

FReply UPauseMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey PressedKey = InKeyEvent.GetKey();
	if (PressedKey == EKeys::Escape || PressedKey == EKeys::Gamepad_Special_Right || PressedKey == EKeys::Gamepad_FaceButton_Right)
	{
		HandleEscapeAction();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UPauseMenuWidget::HandleEscapeAction()
{
	if (ContentSwitcher && ContentSwitcher->GetActiveWidgetIndex() != PanelIndex_Main)
	{
		OnBackToPauseMainClicked();
	}
	else
	{
		OnResumeClicked();
	}
}

void UPauseMenuWidget::FocusInitialButton()
{
	if (ContentSwitcher)
	{
		ContentSwitcher->SetActiveWidgetIndex(PanelIndex_Main);
	}
	if (ResumeButton)
	{
		ResumeButton->SetKeyboardFocus();
	}
}

void UPauseMenuWidget::BuildPauseMainPanel()
{
	UVerticalBox* MainBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PauseMainPanelBox"));
	ContentSwitcher->AddChild(MainBox);

	auto AddMenuButton = [&](const FText& Label, FName WidgetName) -> UButton*
	{
		USizeBox* BtnSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), WidgetName);
		BtnSizeBox->SetWidthOverride(PauseMenuStyle::MainButtonWidth);
		BtnSizeBox->SetMinDesiredHeight(PauseMenuStyle::ButtonMinHeight);

		UButton* Btn = CreateStyledButton(BtnSizeBox, Label, PauseMenuStyle::ButtonFontSize);
		BtnSizeBox->AddChild(Btn);

		if (UVerticalBoxSlot* BoxSlot = MainBox->AddChildToVerticalBox(BtnSizeBox))
		{
			BoxSlot->SetHorizontalAlignment(HAlign_Left);
			BoxSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, PauseMenuStyle::ButtonVerticalPadding));
		}
		return Btn;
	};

	// Sequencia estrita de botoes:
	// 1. "Retornar"
	// 2. "Configurações"
	// 3. "Menu Principal"
	// 4. "Sair"
	ResumeButton = AddMenuButton(LOCTEXT("Btn_Resume", "Retornar"), TEXT("ResumeBtnSizeBox"));
	SettingsButton = AddMenuButton(LOCTEXT("Btn_Settings", "Configurações"), TEXT("PauseSettingsBtnSizeBox"));
	MainMenuButton = AddMenuButton(LOCTEXT("Btn_MainMenu", "Menu Principal"), TEXT("MainMenuBtnSizeBox"));
	QuitButton = AddMenuButton(LOCTEXT("Btn_Quit", "Sair"), TEXT("PauseQuitBtnSizeBox"));
}

UBorder* UPauseMenuWidget::CreateSubmenuCardContainer(FName CardName)
{
	UBorder* CardBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), CardName);

	FSlateBrush CardBrush;
	CardBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
	CardBrush.TintColor = FSlateColor(PauseMenuStyle::SubmenuCardBgColor);
	CardBrush.OutlineSettings.CornerRadii = FVector4(4.0f, 4.0f, 4.0f, 4.0f);
	CardBrush.OutlineSettings.Color = FSlateColor(PauseMenuStyle::ButtonSubtleBorderColor);
	CardBrush.OutlineSettings.Width = 1.5f;
	CardBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
	CardBrush.OutlineSettings.bUseBrushTransparency = true;

	CardBorder->SetBrush(CardBrush);
	CardBorder->SetPadding(FMargin(24.0f, 20.0f));
	return CardBorder;
}

void UPauseMenuWidget::BuildSettingsPanel()
{
	USizeBox* CardSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PauseSettingsCardSizeBox"));
	CardSizeBox->SetWidthOverride(PauseMenuStyle::SubmenuPanelWidth);
	ContentSwitcher->AddChild(CardSizeBox);

	UBorder* CardBorder = CreateSubmenuCardContainer(TEXT("PauseSettingsCardBorder"));
	CardSizeBox->AddChild(CardBorder);

	UVerticalBox* SettingsPanelBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PauseSettingsPanelBox"));
	CardBorder->AddChild(SettingsPanelBox);

	UTextBlock* HeaderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PauseSettingsHeaderText"));
	HeaderText->SetText(LOCTEXT("Header_Settings", "CONFIGURAÇÕES"));
	HeaderText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", PauseMenuStyle::PanelHeaderFontSize));
	HeaderText->SetColorAndOpacity(FSlateColor(PauseMenuStyle::PeachAccentColor));
	HeaderText->SetJustification(ETextJustify::Center);

	if (UVerticalBoxSlot* HeaderSlot = SettingsPanelBox->AddChildToVerticalBox(HeaderText))
	{
		HeaderSlot->SetHorizontalAlignment(HAlign_Center);
		HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
	}

	auto AddSliderRow = [&](const FText& Label, float MinVal, float MaxVal, TObjectPtr<USlider>& OutSlider, TObjectPtr<UTextBlock>& OutValueText, FName RowName)
	{
		UVerticalBox* RowBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), RowName);

		UHorizontalBox* TopLine = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UTextBlock* LabelBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		LabelBlock->SetText(Label);
		LabelBlock->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", PauseMenuStyle::LabelFontSize));
		LabelBlock->SetColorAndOpacity(FSlateColor(PauseMenuStyle::CreamTextColor));

		OutValueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		OutValueText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", PauseMenuStyle::LabelFontSize));
		OutValueText->SetColorAndOpacity(FSlateColor(PauseMenuStyle::PeachAccentColor));
		OutValueText->SetJustification(ETextJustify::Right);

		if (UHorizontalBoxSlot* LabelSlot = TopLine->AddChildToHorizontalBox(LabelBlock))
		{
			LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			LabelSlot->SetHorizontalAlignment(HAlign_Left);
		}
		if (UHorizontalBoxSlot* ValSlot = TopLine->AddChildToHorizontalBox(OutValueText))
		{
			ValSlot->SetHorizontalAlignment(HAlign_Right);
		}

		RowBox->AddChildToVerticalBox(TopLine);

		OutSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
		OutSlider->SetMinValue(MinVal);
		OutSlider->SetMaxValue(MaxVal);
		OutSlider->SetValue(1.0f);
		OutSlider->SetSliderBarColor(PauseMenuStyle::ButtonNormalBgColor);
		OutSlider->SetSliderHandleColor(PauseMenuStyle::PeachAccentColor);

		if (UVerticalBoxSlot* SliderSlot = RowBox->AddChildToVerticalBox(OutSlider))
		{
			SliderSlot->SetPadding(FMargin(0.0f, 5.0f, 0.0f, 0.0f));
		}

		if (UVerticalBoxSlot* RowSlot = SettingsPanelBox->AddChildToVerticalBox(RowBox))
		{
			RowSlot->SetPadding(FMargin(0.0f, 6.0f));
		}
	};

	// 1. Volume Geral (0 a 1)
	AddSliderRow(LOCTEXT("Setting_Volume", "Volume Geral"), 0.0f, 1.0f, VolumeSlider, VolumeValueText, TEXT("PauseVolumeRow"));

	// 2. Sensibilidade (0.1 a 5.0)
	AddSliderRow(LOCTEXT("Setting_Sensitivity", "Sensibilidade"), 0.1f, 5.0f, SensitivitySlider, SensitivityValueText, TEXT("PauseSensitivityRow"));

	// 3. Campo de Visao / FOV (70 a 120 graus)
	AddSliderRow(LOCTEXT("Setting_FOV", "Campo de Visão (FOV)"), 70.0f, 120.0f, FOVSlider, FOVValueText, TEXT("PauseFOVRow"));

	auto AddSelectorRow = [&](const FText& Label, TObjectPtr<UButton>& OutPrevBtn, TObjectPtr<UTextBlock>& OutValText, TObjectPtr<UButton>& OutNextBtn, FName RowName)
	{
		UVerticalBox* RowBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), RowName);

		UTextBlock* LabelBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		LabelBlock->SetText(Label);
		LabelBlock->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", PauseMenuStyle::LabelFontSize));
		LabelBlock->SetColorAndOpacity(FSlateColor(PauseMenuStyle::CreamTextColor));

		if (UVerticalBoxSlot* LblSlot = RowBox->AddChildToVerticalBox(LabelBlock))
		{
			LblSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 5.0f));
		}

		UHorizontalBox* SelectorLine = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

		USizeBox* PrevSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		PrevSize->SetMinDesiredWidth(PauseMenuStyle::SmallButtonMinWidth);
		PrevSize->SetMinDesiredHeight(38.0f);
		OutPrevBtn = CreateStyledButton(PrevSize, FText::FromString(TEXT("<")), PauseMenuStyle::LabelFontSize);
		PrevSize->AddChild(OutPrevBtn);

		OutValText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		OutValText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", PauseMenuStyle::LabelFontSize));
		OutValText->SetColorAndOpacity(FSlateColor(PauseMenuStyle::WarmSecondaryTextColor));
		OutValText->SetJustification(ETextJustify::Center);

		USizeBox* NextSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		NextSize->SetMinDesiredWidth(PauseMenuStyle::SmallButtonMinWidth);
		NextSize->SetMinDesiredHeight(38.0f);
		OutNextBtn = CreateStyledButton(NextSize, FText::FromString(TEXT(">")), PauseMenuStyle::LabelFontSize);
		NextSize->AddChild(OutNextBtn);

		if (UHorizontalBoxSlot* PrevSlot = SelectorLine->AddChildToHorizontalBox(PrevSize))
		{
			PrevSlot->SetVerticalAlignment(VAlign_Center);
		}
		if (UHorizontalBoxSlot* CenterSlot = SelectorLine->AddChildToHorizontalBox(OutValText))
		{
			CenterSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			CenterSlot->SetHorizontalAlignment(HAlign_Center);
			CenterSlot->SetVerticalAlignment(VAlign_Center);
		}
		if (UHorizontalBoxSlot* NextSlot = SelectorLine->AddChildToHorizontalBox(NextSize))
		{
			NextSlot->SetVerticalAlignment(VAlign_Center);
		}

		RowBox->AddChildToVerticalBox(SelectorLine);

		if (UVerticalBoxSlot* RowSlot = SettingsPanelBox->AddChildToVerticalBox(RowBox))
		{
			RowSlot->SetPadding(FMargin(0.0f, 6.0f));
		}
	};

	// 4. Modo de Janela
	AddSelectorRow(LOCTEXT("Setting_WindowMode", "Modo de Janela"), WindowModePrevButton, WindowModeValueText, WindowModeNextButton, TEXT("PauseWindowModeRow"));

	// 5. Resolucao
	AddSelectorRow(LOCTEXT("Setting_Resolution", "Resolução"), ResolutionPrevButton, ResolutionValueText, ResolutionNextButton, TEXT("PauseResolutionRow"));

	// 6. Qualidade Grafica (Baixa, Media, Alta, Epica)
	AddSelectorRow(LOCTEXT("Setting_GraphicsQuality", "Qualidade Gráfica"), GraphicsQualityPrevButton, GraphicsQualityValueText, GraphicsQualityNextButton, TEXT("PauseGraphicsQualityRow"));

	USizeBox* ApplySizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PauseSettingsApplySizeBox"));
	ApplySizeBox->SetMinDesiredHeight(PauseMenuStyle::ButtonMinHeight);
	SettingsApplyButton = CreateStyledButton(ApplySizeBox, LOCTEXT("Btn_Apply", "Aplicar"), PauseMenuStyle::ButtonFontSize);
	ApplySizeBox->AddChild(SettingsApplyButton);

	if (UVerticalBoxSlot* ApplySlot = SettingsPanelBox->AddChildToVerticalBox(ApplySizeBox))
	{
		ApplySlot->SetHorizontalAlignment(HAlign_Fill);
		ApplySlot->SetPadding(FMargin(8.0f, 14.0f, 8.0f, 6.0f));
	}

	USizeBox* BackSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PauseSettingsBackSizeBox"));
	BackSizeBox->SetMinDesiredHeight(PauseMenuStyle::ButtonMinHeight);
	SettingsBackButton = CreateStyledButton(BackSizeBox, LOCTEXT("Btn_Back", "Voltar"), PauseMenuStyle::ButtonFontSize);
	BackSizeBox->AddChild(SettingsBackButton);

	if (UVerticalBoxSlot* BackSlot = SettingsPanelBox->AddChildToVerticalBox(BackSizeBox))
	{
		BackSlot->SetHorizontalAlignment(HAlign_Fill);
		BackSlot->SetPadding(FMargin(8.0f, 4.0f));
	}
}

void UPauseMenuWidget::LoadSettingsToUI()
{
	UMainMenuSaveGame* SaveData = UMainMenuSaveGame::LoadOrCreate();
	if (SaveData)
	{
		CurrentVolume = FMath::Clamp(SaveData->MasterVolume, 0.0f, 1.0f);
		CurrentSensitivity = FMath::Clamp(SaveData->MouseSensitivity, 0.1f, 5.0f);
		CurrentFOV = FMath::Clamp(SaveData->FieldOfView, 70.0f, 120.0f);
		CurrentWindowModeIndex = FMath::Clamp(SaveData->PreferredWindowMode, 0, 2);
		CurrentGraphicsQualityIndex = FMath::Clamp(SaveData->PreferredGraphicsQuality, 0, 3);
	}

	if (UGameUserSettings* UserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		const EWindowMode::Type ActiveMode = UserSettings->GetFullscreenMode();
		if (ActiveMode == EWindowMode::Fullscreen)
		{
			CurrentWindowModeIndex = 0;
		}
		else if (ActiveMode == EWindowMode::WindowedFullscreen)
		{
			CurrentWindowModeIndex = 1;
		}
		else
		{
			CurrentWindowModeIndex = 2;
		}

		FIntPoint CurrentRes = UserSettings->GetScreenResolution();
		if (CurrentRes.X <= 0 || CurrentRes.Y <= 0)
		{
			CurrentRes = SaveData ? SaveData->PreferredResolution : FIntPoint(1920, 1080);
		}

		int32 FoundIdx = AvailableResolutions.IndexOfByKey(CurrentRes);
		if (FoundIdx == INDEX_NONE)
		{
			FoundIdx = AvailableResolutions.Add(CurrentRes);
		}
		CurrentResolutionIndex = FoundIdx;
	}

	if (VolumeSlider)
	{
		VolumeSlider->SetValue(CurrentVolume);
	}
	if (SensitivitySlider)
	{
		SensitivitySlider->SetValue(CurrentSensitivity);
	}
	if (FOVSlider)
	{
		FOVSlider->SetValue(CurrentFOV);
	}

	UpdateSettingsLabels();
}

void UPauseMenuWidget::UpdateSettingsLabels()
{
	if (VolumeValueText)
	{
		const int32 VolumePercent = FMath::RoundToInt(CurrentVolume * 100.0f);
		VolumeValueText->SetText(FText::Format(LOCTEXT("Volume_PercentFmt", "{0}%"), FText::AsNumber(VolumePercent)));
	}

	if (SensitivityValueText)
	{
		FNumberFormattingOptions FmtOpts;
		FmtOpts.MinimumFractionalDigits = 2;
		FmtOpts.MaximumFractionalDigits = 2;
		SensitivityValueText->SetText(FText::AsNumber(CurrentSensitivity, &FmtOpts));
	}

	if (FOVValueText)
	{
		const int32 FOVInt = FMath::RoundToInt(CurrentFOV);
		FOVValueText->SetText(FText::Format(LOCTEXT("FOV_DegFmt", "{0}°"), FText::AsNumber(FOVInt)));
	}

	if (WindowModeValueText)
	{
		FText ModeText;
		switch (CurrentWindowModeIndex)
		{
		case 0:
			ModeText = LOCTEXT("WinMode_Fullscreen", "Tela Cheia");
			break;
		case 1:
			ModeText = LOCTEXT("WinMode_WindowedFullscreen", "Janela sem Borda");
			break;
		default:
			ModeText = LOCTEXT("WinMode_Windowed", "Janela");
			break;
		}
		WindowModeValueText->SetText(ModeText);
	}

	if (ResolutionValueText && AvailableResolutions.IsValidIndex(CurrentResolutionIndex))
	{
		const FIntPoint& Res = AvailableResolutions[CurrentResolutionIndex];
		ResolutionValueText->SetText(FText::FromString(FString::Printf(TEXT("%d x %d"), Res.X, Res.Y)));
	}

	if (GraphicsQualityValueText)
	{
		FText QualityText;
		switch (CurrentGraphicsQualityIndex)
		{
		case 0:
			QualityText = LOCTEXT("Quality_Low", "Baixa");
			break;
		case 1:
			QualityText = LOCTEXT("Quality_Medium", "Média");
			break;
		case 2:
			QualityText = LOCTEXT("Quality_High", "Alta");
			break;
		default:
			QualityText = LOCTEXT("Quality_Epic", "Épica");
			break;
		}
		GraphicsQualityValueText->SetText(QualityText);
	}
}

void UPauseMenuWidget::SaveAndApplyRealtimeSettings(bool bApplyVideoMode)
{
	UWorld* World = GetWorld();

	const FIntPoint SelectedRes = AvailableResolutions.IsValidIndex(CurrentResolutionIndex)
		? AvailableResolutions[CurrentResolutionIndex]
		: FIntPoint(1920, 1080);

	// 1. Video / Graficos / Resolucao via UGameUserSettings::GetGameUserSettings()
	if (bApplyVideoMode)
	{
		if (UGameUserSettings* UserSettings = UGameUserSettings::GetGameUserSettings())
		{
			EWindowMode::Type TargetWindowMode = EWindowMode::WindowedFullscreen;
			if (CurrentWindowModeIndex == 0)
			{
				TargetWindowMode = EWindowMode::Fullscreen;
			}
			else if (CurrentWindowModeIndex == 1)
			{
				TargetWindowMode = EWindowMode::WindowedFullscreen;
			}
			else
			{
				TargetWindowMode = EWindowMode::Windowed;
			}

			UserSettings->SetFullscreenMode(TargetWindowMode);
			UserSettings->SetScreenResolution(SelectedRes);
			UserSettings->SetOverallScalabilityLevel(FMath::Clamp(CurrentGraphicsQualityIndex, 0, 3));
			UserSettings->ApplySettings(false);
			UserSettings->SaveSettings();
		}
	}

	// 2. Persistencia na GameInstance (em memoria para as proximas fases da run)
	if (World)
	{
		if (USurfGameInstance* SurfGI = Cast<USurfGameInstance>(World->GetGameInstance()))
		{
			SurfGI->MasterVolume = CurrentVolume;
			SurfGI->MouseSensitivity = CurrentSensitivity;
			SurfGI->FieldOfView = CurrentFOV;
			SurfGI->PreferredWindowMode = CurrentWindowModeIndex;
			SurfGI->PreferredResolution = SelectedRes;
			SurfGI->PreferredGraphicsQuality = FMath::Clamp(CurrentGraphicsQualityIndex, 0, 3);
		}
	}

	// 3. Persistencia no SaveGame (.sav)
	if (UMainMenuSaveGame* SaveData = UMainMenuSaveGame::LoadOrCreate())
	{
		SaveData->MasterVolume = CurrentVolume;
		SaveData->MouseSensitivity = CurrentSensitivity;
		SaveData->FieldOfView = CurrentFOV;
		SaveData->PreferredWindowMode = CurrentWindowModeIndex;
		SaveData->PreferredResolution = SelectedRes;
		SaveData->PreferredGraphicsQuality = FMath::Clamp(CurrentGraphicsQualityIndex, 0, 3);
		SaveData->SaveToSlot();
	}

	// 4. Aplicacao imediata em tempo real (Audio, Sensibilidade e FOV no PlayerController / Character da fase atual)
	UMainMenuSaveGame::ApplyRealtimePreferencesToWorld(World, CurrentVolume, CurrentSensitivity, CurrentFOV);
}

void UPauseMenuWidget::ShowStatusMessage(const FText& Message, bool bIsWarning)
{
	if (!StatusMessageText)
	{
		return;
	}

	if (Message.IsEmpty())
	{
		StatusMessageText->SetText(FText::GetEmpty());
		StatusMessageText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	StatusMessageText->SetColorAndOpacity(FSlateColor(bIsWarning ? PauseMenuStyle::PeachAccentColor : PauseMenuStyle::CreamTextColor));
	StatusMessageText->SetText(Message);
	StatusMessageText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UPauseMenuWidget::SwitchToPanel(int32 PanelIndex, UButton* DefaultFocusButton)
{
	ShowStatusMessage(FText::GetEmpty());

	if (ContentSwitcher)
	{
		ContentSwitcher->SetActiveWidgetIndex(PanelIndex);
	}

	if (DefaultFocusButton)
	{
		DefaultFocusButton->SetKeyboardFocus();
	}
}

void UPauseMenuWidget::OnResumeClicked()
{
	if (AMainMenuPlayerController* MenuPC = Cast<AMainMenuPlayerController>(GetOwningPlayer()))
	{
		MenuPC->ClosePauseMenu();
	}
	else if (APlayerController* PC = GetOwningPlayer())
	{
		UGameplayStatics::SetGamePaused(this, false);
		FInputModeGameOnly GameInputMode;
		GameInputMode.SetConsumeCaptureMouseDown(false);
		PC->SetInputMode(GameInputMode);
		PC->bShowMouseCursor = false;
		PC->ResetIgnoreInputFlags();
		RemoveFromParent();
	}
}

void UPauseMenuWidget::OnSettingsClicked()
{
	LoadSettingsToUI();
	SwitchToPanel(PanelIndex_Settings, SettingsApplyButton.Get());
}

void UPauseMenuWidget::OnMainMenuClicked()
{
	RemoveFromParent();
	USurfGameInstance::ReturnToMainMenuOnMap1(this);
}

void UPauseMenuWidget::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UPauseMenuWidget::OnBackToPauseMainClicked()
{
	SwitchToPanel(PanelIndex_Main, ResumeButton.Get());
}

void UPauseMenuWidget::OnVolumeChanged(float NewValue)
{
	CurrentVolume = FMath::Clamp(NewValue, 0.0f, 1.0f);
	UpdateSettingsLabels();
	SaveAndApplyRealtimeSettings(false);
}

void UPauseMenuWidget::OnSensitivityChanged(float NewValue)
{
	CurrentSensitivity = FMath::Clamp(NewValue, 0.1f, 5.0f);
	UpdateSettingsLabels();
	SaveAndApplyRealtimeSettings(false);
}

void UPauseMenuWidget::OnFOVChanged(float NewValue)
{
	CurrentFOV = FMath::Clamp(NewValue, 70.0f, 120.0f);
	UpdateSettingsLabels();
	SaveAndApplyRealtimeSettings(false);
}

void UPauseMenuWidget::OnWindowModePrevClicked()
{
	CurrentWindowModeIndex = (CurrentWindowModeIndex + 2) % 3;
	UpdateSettingsLabels();
	SaveAndApplyRealtimeSettings(true);
}

void UPauseMenuWidget::OnWindowModeNextClicked()
{
	CurrentWindowModeIndex = (CurrentWindowModeIndex + 1) % 3;
	UpdateSettingsLabels();
	SaveAndApplyRealtimeSettings(true);
}

void UPauseMenuWidget::OnResolutionPrevClicked()
{
	if (AvailableResolutions.Num() > 0)
	{
		CurrentResolutionIndex = (CurrentResolutionIndex - 1 + AvailableResolutions.Num()) % AvailableResolutions.Num();
		UpdateSettingsLabels();
		SaveAndApplyRealtimeSettings(true);
	}
}

void UPauseMenuWidget::OnResolutionNextClicked()
{
	if (AvailableResolutions.Num() > 0)
	{
		CurrentResolutionIndex = (CurrentResolutionIndex + 1) % AvailableResolutions.Num();
		UpdateSettingsLabels();
		SaveAndApplyRealtimeSettings(true);
	}
}

void UPauseMenuWidget::OnGraphicsQualityPrevClicked()
{
	CurrentGraphicsQualityIndex = (CurrentGraphicsQualityIndex + 3) % 4;
	UpdateSettingsLabels();
	SaveAndApplyRealtimeSettings(true);
}

void UPauseMenuWidget::OnGraphicsQualityNextClicked()
{
	CurrentGraphicsQualityIndex = (CurrentGraphicsQualityIndex + 1) % 4;
	UpdateSettingsLabels();
	SaveAndApplyRealtimeSettings(true);
}

void UPauseMenuWidget::OnSettingsApplyClicked()
{
	SaveAndApplyRealtimeSettings(true);
	ShowStatusMessage(LOCTEXT("Msg_PauseSettingsSaved", "Configurações aplicadas em tempo real e salvas!"), false);
}

UButton* UPauseMenuWidget::CreateStyledButton(UWidget* Outer, const FText& LabelText, int32 FontSize)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());

	auto MakeParallelogramBrush = [](const FLinearColor& FillColor) -> FSlateBrush
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(FillColor);
		Brush.OutlineSettings.CornerRadii = FVector4(
			PauseMenuStyle::ButtonCornerRadius,
			PauseMenuStyle::ButtonCornerRadius,
			PauseMenuStyle::ButtonCornerRadius,
			PauseMenuStyle::ButtonCornerRadius);
		Brush.OutlineSettings.Color = FSlateColor(PauseMenuStyle::ButtonSubtleBorderColor);
		Brush.OutlineSettings.Width = PauseMenuStyle::ButtonBorderThickness;
		Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		Brush.OutlineSettings.bUseBrushTransparency = true;
		return Brush;
	};

	FButtonStyle BtnStyle = Button->GetStyle();
	BtnStyle.SetNormal(MakeParallelogramBrush(PauseMenuStyle::ButtonNormalBgColor));
	BtnStyle.SetHovered(MakeParallelogramBrush(PauseMenuStyle::ButtonHoverBgColor));
	BtnStyle.SetPressed(MakeParallelogramBrush(PauseMenuStyle::ButtonPressedBgColor));
	BtnStyle.SetNormalPadding(FMargin(22.0f, 10.0f));
	BtnStyle.SetPressedPadding(FMargin(22.0f, 11.5f, 22.0f, 8.5f));
	Button->SetStyle(BtnStyle);

	Button->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	Button->SetRenderShear(FVector2D(PauseMenuStyle::ButtonShearAngleDeg, 0.0f));

	UTextBlock* BtnLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	BtnLabel->SetText(LabelText);

	FSlateFontInfo BtnFont = FCoreStyle::GetDefaultFontStyle("Bold", FontSize);
	BtnFont.LetterSpacing = PauseMenuStyle::ButtonLetterSpacing;
	BtnLabel->SetFont(BtnFont);
	BtnLabel->SetColorAndOpacity(FSlateColor(PauseMenuStyle::CreamTextColor));
	BtnLabel->SetJustification(ETextJustify::Center);

	BtnLabel->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	BtnLabel->SetRenderShear(FVector2D(-PauseMenuStyle::ButtonShearAngleDeg, 0.0f));

	if (UButtonSlot* BtnSlot = Cast<UButtonSlot>(Button->AddChild(BtnLabel)))
	{
		BtnSlot->SetHorizontalAlignment(HAlign_Center);
		BtnSlot->SetVerticalAlignment(VAlign_Center);
	}

	AllStyledButtons.AddUnique(Button);
	return Button;
}

#undef LOCTEXT_NAMESPACE
