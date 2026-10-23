#include "MainMenuWidget.h"
#include "MainMenuSettings.h"
#include "MainMenuSaveGame.h"
#include "MainMenuPlayerController.h"
#include "Framework/Application/SlateApplication.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/ScrollBox.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/TextBlock.h"
#include "Components/Slider.h"
#include "Components/Image.h"
#include "Components/Spacer.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"
#include "Engine/Engine.h"
#include "AudioDevice.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Styling/CoreStyle.h"
#include "InputCoreTypes.h"
#include "MediaPlayer.h"
#include "MediaTexture.h"
#include "FileMediaSource.h"
#include "ImageUtils.h"
#include "ImageCore.h"
#include "Misc/Paths.h"

#define LOCTEXT_NAMESPACE "MainMenu"

namespace MainMenuStyle
{
	// Cores do fundo (gradiente azul-marinho escuro para violeta)
	static const FLinearColor GradientTopColor(0.02f, 0.04f, 0.14f, 1.0f);
	static const FLinearColor GradientBottomColor(0.18f, 0.03f, 0.32f, 1.0f);

	// Cores de texto e destaque
	static const FLinearColor CyanAccentColor(0.0f, 0.90f, 1.0f, 1.0f);
	static const FLinearColor PrimaryTextColor(0.94f, 0.97f, 1.0f, 1.0f);
	static const FLinearColor SecondaryTextColor(0.68f, 0.80f, 0.92f, 1.0f);
	static const FLinearColor WarningTextColor(1.0f, 0.75f, 0.20f, 1.0f);

	// Cores dos botoes (retangulo arredondado escuro semitransparente com borda fina ciano)
	static const FLinearColor ButtonNormalBgColor(0.03f, 0.06f, 0.15f, 0.82f);
	static const FLinearColor ButtonHoverBgColor(0.08f, 0.24f, 0.42f, 0.92f);
	static const FLinearColor ButtonPressedBgColor(0.01f, 0.03f, 0.08f, 0.96f);
	static const FLinearColor ButtonBorderCyanColor(0.0f, 0.85f, 1.0f, 0.95f);

	// Dimensoes e espacamentos
	static constexpr float MenuContainerWidth = 480.0f;
	static constexpr float TitleLogoDisplayWidth = 480.0f;
	static constexpr float ButtonMinHeight = 50.0f;
	static constexpr float SmallButtonMinWidth = 48.0f;
	static constexpr float ButtonCornerRadius = 8.0f;
	static constexpr float ButtonBorderThickness = 1.5f;
	static constexpr float ButtonVerticalSpacing = 6.0f;
	static constexpr float SectionBottomSpacing = 24.0f;
	static constexpr float LevelsScrollMaxHeight = 280.0f;

	// Tamanhos de fonte
	static constexpr int32 TitleFontSize = 54;
	static constexpr int32 PanelHeaderFontSize = 24;
	static constexpr int32 ButtonFontSize = 18;
	static constexpr int32 LabelFontSize = 15;
	static constexpr int32 StatusFontSize = 14;
}

void UMainMenuLevelClickProxy::Setup(UMainMenuWidget* InOwner, int32 InLevelIndex)
{
	OwnerWidget = InOwner;
	LevelIndex = InLevelIndex;
}

void UMainMenuLevelClickProxy::HandleClicked()
{
	if (OwnerWidget)
	{
		OwnerWidget->OpenLevelByIndex(LevelIndex);
	}
}

UMainMenuWidget::UMainMenuWidget(const FObjectInitializer& ObjectInitializer)
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

TSharedRef<SWidget> UMainMenuWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("MainMenuWidgetTree"));
	}

	// Painel raiz ocupando toda a tela (responsivo para 16:9, ultrawide, etc.)
	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	// Fundo: imagem em tela cheia que recebe o video em looping (ou gradiente procedural como fallback)
	BackgroundImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BackgroundVideoImage"));
	BackgroundGradientTexture = CreateProceduralGradientTexture();
	if (BackgroundGradientTexture)
	{
		BackgroundImage->SetBrushFromTexture(BackgroundGradientTexture, false);
	}
	else
	{
		BackgroundImage->SetColorAndOpacity(MainMenuStyle::GradientTopColor);
	}

	if (UCanvasPanelSlot* BgSlot = RootCanvas->AddChildToCanvas(BackgroundImage))
	{
		BgSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		BgSlot->SetOffsets(FMargin(0.0f));
		BgSlot->SetZOrder(0);
	}

	// Container centralizado
	USizeBox* CenterSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CenterSizeBox"));
	CenterSizeBox->SetWidthOverride(MainMenuStyle::MenuContainerWidth);

	if (UCanvasPanelSlot* CenterSlot = RootCanvas->AddChildToCanvas(CenterSizeBox))
	{
		CenterSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		CenterSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CenterSlot->SetAutoSize(true);
		CenterSlot->SetZOrder(1);
	}

	UVerticalBox* MainVerticalLayout = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainVerticalLayout"));
	CenterSizeBox->AddChild(MainVerticalLayout);

	// Logo PNG "MOMENTUM" no topo substituindo o texto antigo
	FVector2D LogoDisplaySize(MainMenuStyle::TitleLogoDisplayWidth, 120.0f);
	TitleLogoTexture = LoadTrimmedTitleLogoTexture(LogoDisplaySize);

	USizeBox* TitleLogoSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("TitleLogoSizeBox"));
	TitleLogoSizeBox->SetWidthOverride(LogoDisplaySize.X);
	TitleLogoSizeBox->SetHeightOverride(LogoDisplaySize.Y);

	TitleLogoImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("TitleLogoImage"));
	if (TitleLogoTexture)
	{
		FSlateBrush LogoBrush;
		LogoBrush.DrawAs = ESlateBrushDrawType::Image;
		LogoBrush.SetResourceObject(TitleLogoTexture);
		LogoBrush.ImageSize = LogoDisplaySize;
		TitleLogoImage->SetBrush(LogoBrush);
	}
	TitleLogoSizeBox->AddChild(TitleLogoImage);

	if (UVerticalBoxSlot* TitleSlot = MainVerticalLayout->AddChildToVerticalBox(TitleLogoSizeBox))
	{
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, MainMenuStyle::SectionBottomSpacing));
	}

	// Switcher de paineis (0: Menu Principal, 1: Fases, 2: Config, 3: Creditos)
	// TODO: Adicionar animacoes de transicao (fade/slide) ao alternar paineis no ContentSwitcher.
	ContentSwitcher = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("ContentSwitcher"));
	if (UVerticalBoxSlot* SwitcherSlot = MainVerticalLayout->AddChildToVerticalBox(ContentSwitcher))
	{
		SwitcherSlot->SetHorizontalAlignment(HAlign_Fill);
		SwitcherSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	// Mensagem de status/aviso na parte inferior do container
	StatusMessageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusMessageText"));
	StatusMessageText->SetText(FText::GetEmpty());
	StatusMessageText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", MainMenuStyle::StatusFontSize));
	StatusMessageText->SetColorAndOpacity(FSlateColor(MainMenuStyle::WarningTextColor));
	StatusMessageText->SetJustification(ETextJustify::Center);
	StatusMessageText->SetAutoWrapText(true);
	StatusMessageText->SetVisibility(ESlateVisibility::Collapsed);

	if (UVerticalBoxSlot* StatusSlot = MainVerticalLayout->AddChildToVerticalBox(StatusMessageText))
	{
		StatusSlot->SetHorizontalAlignment(HAlign_Fill);
		StatusSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
	}

	BuildMainPanel();
	BuildLevelsPanel();
	BuildSettingsPanel();
	BuildCreditsPanel();

	ContentSwitcher->SetActiveWidgetIndex(PanelIndex_Main);

	return Super::RebuildWidget();
}

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (PlayButton)
	{
		PlayButton->OnClicked.RemoveAll(this);
		PlayButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnPlayClicked);
	}
	if (LevelsButton)
	{
		LevelsButton->OnClicked.RemoveAll(this);
		LevelsButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnLevelsClicked);
	}
	if (SettingsButton)
	{
		SettingsButton->OnClicked.RemoveAll(this);
		SettingsButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnSettingsClicked);
	}
	if (CreditsButton)
	{
		CreditsButton->OnClicked.RemoveAll(this);
		CreditsButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnCreditsClicked);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.RemoveAll(this);
		QuitButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnQuitClicked);
	}

	if (LevelsBackButton)
	{
		LevelsBackButton->OnClicked.RemoveAll(this);
		LevelsBackButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnBackToMainClicked);
	}

	if (VolumeSlider)
	{
		VolumeSlider->OnValueChanged.RemoveAll(this);
		VolumeSlider->OnValueChanged.AddDynamic(this, &UMainMenuWidget::OnVolumeChanged);
	}
	if (SensitivitySlider)
	{
		SensitivitySlider->OnValueChanged.RemoveAll(this);
		SensitivitySlider->OnValueChanged.AddDynamic(this, &UMainMenuWidget::OnSensitivityChanged);
	}
	if (WindowModePrevButton)
	{
		WindowModePrevButton->OnClicked.RemoveAll(this);
		WindowModePrevButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnWindowModePrevClicked);
	}
	if (WindowModeNextButton)
	{
		WindowModeNextButton->OnClicked.RemoveAll(this);
		WindowModeNextButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnWindowModeNextClicked);
	}
	if (ResolutionPrevButton)
	{
		ResolutionPrevButton->OnClicked.RemoveAll(this);
		ResolutionPrevButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnResolutionPrevClicked);
	}
	if (ResolutionNextButton)
	{
		ResolutionNextButton->OnClicked.RemoveAll(this);
		ResolutionNextButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnResolutionNextClicked);
	}
	if (SettingsApplyButton)
	{
		SettingsApplyButton->OnClicked.RemoveAll(this);
		SettingsApplyButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnSettingsApplyClicked);
	}
	if (SettingsBackButton)
	{
		SettingsBackButton->OnClicked.RemoveAll(this);
		SettingsBackButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnBackToMainClicked);
	}

	if (CreditsBackButton)
	{
		CreditsBackButton->OnClicked.RemoveAll(this);
		CreditsBackButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnBackToMainClicked);
	}

	StartBackgroundVideo();
	LoadSettingsToUI();
	ApplyVolumeSetting(CurrentVolume);
	RefreshLevelsList();
	FocusInitialButton();
}

void UMainMenuWidget::NativeDestruct()
{
	StopBackgroundVideo();
	Super::NativeDestruct();
}

FReply UMainMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey PressedKey = InKeyEvent.GetKey();
	if (PressedKey == EKeys::Escape || PressedKey == EKeys::Gamepad_FaceButton_Right)
	{
		if (ContentSwitcher && ContentSwitcher->GetActiveWidgetIndex() != PanelIndex_Main)
		{
			OnBackToMainClicked();
			return FReply::Handled();
		}
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UMainMenuWidget::FocusInitialButton()
{
	if (PlayButton)
	{
		PlayButton->SetKeyboardFocus();
	}
}

void UMainMenuWidget::BuildMainPanel()
{
	UVerticalBox* MainBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainPanelBox"));
	ContentSwitcher->AddChild(MainBox);

	auto AddMenuButton = [&](const FText& Label, FName WidgetName) -> UButton*
	{
		USizeBox* BtnSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), WidgetName);
		BtnSizeBox->SetMinDesiredHeight(MainMenuStyle::ButtonMinHeight);

		UButton* Btn = CreateStyledButton(BtnSizeBox, Label, MainMenuStyle::ButtonFontSize);
		BtnSizeBox->AddChild(Btn);

		if (UVerticalBoxSlot* BoxSlot = MainBox->AddChildToVerticalBox(BtnSizeBox))
		{
			BoxSlot->SetHorizontalAlignment(HAlign_Fill);
			BoxSlot->SetPadding(FMargin(0.0f, MainMenuStyle::ButtonVerticalSpacing));
		}
		return Btn;
	};

	// Ordem vertical centralizada obrigatoria: Jogar, Fases, Config, Creditos, Sair
	PlayButton = AddMenuButton(LOCTEXT("Btn_Play", "Jogar"), TEXT("PlayBtnSizeBox"));
	LevelsButton = AddMenuButton(LOCTEXT("Btn_Levels", "Fases"), TEXT("LevelsBtnSizeBox"));
	SettingsButton = AddMenuButton(LOCTEXT("Btn_Settings", "Config"), TEXT("SettingsBtnSizeBox"));
	CreditsButton = AddMenuButton(LOCTEXT("Btn_Credits", "Créditos"), TEXT("CreditsBtnSizeBox"));
	QuitButton = AddMenuButton(LOCTEXT("Btn_Quit", "Sair"), TEXT("QuitBtnSizeBox"));

	// TODO: Adicionar botoes/paineis de Multiplayer e Login em iteracoes futuras.
}

void UMainMenuWidget::BuildLevelsPanel()
{
	UVerticalBox* LevelsPanelBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LevelsPanelBox"));
	ContentSwitcher->AddChild(LevelsPanelBox);

	UTextBlock* HeaderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LevelsHeaderText"));
	HeaderText->SetText(LOCTEXT("Header_Levels", "SELECIONAR FASE"));
	HeaderText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", MainMenuStyle::PanelHeaderFontSize));
	HeaderText->SetColorAndOpacity(FSlateColor(MainMenuStyle::CyanAccentColor));
	HeaderText->SetJustification(ETextJustify::Center);

	if (UVerticalBoxSlot* HeaderSlot = LevelsPanelBox->AddChildToVerticalBox(HeaderText))
	{
		HeaderSlot->SetHorizontalAlignment(HAlign_Center);
		HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
	}

	USizeBox* ScrollSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LevelsScrollSizeBox"));
	ScrollSizeBox->SetMaxDesiredHeight(MainMenuStyle::LevelsScrollMaxHeight);

	UScrollBox* ScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("LevelsScrollBox"));
	ScrollSizeBox->AddChild(ScrollBox);

	LevelsListContainer = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LevelsListContainer"));
	ScrollBox->AddChild(LevelsListContainer);

	if (UVerticalBoxSlot* ListSlot = LevelsPanelBox->AddChildToVerticalBox(ScrollSizeBox))
	{
		ListSlot->SetHorizontalAlignment(HAlign_Fill);
		ListSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
	}

	USizeBox* BackSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LevelsBackSizeBox"));
	BackSizeBox->SetMinDesiredHeight(MainMenuStyle::ButtonMinHeight);
	LevelsBackButton = CreateStyledButton(BackSizeBox, LOCTEXT("Btn_Back", "Voltar"), MainMenuStyle::ButtonFontSize);
	BackSizeBox->AddChild(LevelsBackButton);

	if (UVerticalBoxSlot* BackSlot = LevelsPanelBox->AddChildToVerticalBox(BackSizeBox))
	{
		BackSlot->SetHorizontalAlignment(HAlign_Fill);
		BackSlot->SetPadding(FMargin(0.0f, MainMenuStyle::ButtonVerticalSpacing));
	}
}

void UMainMenuWidget::BuildSettingsPanel()
{
	UVerticalBox* SettingsPanelBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsPanelBox"));
	ContentSwitcher->AddChild(SettingsPanelBox);

	UTextBlock* HeaderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SettingsHeaderText"));
	HeaderText->SetText(LOCTEXT("Header_Settings", "CONFIGURAÇÕES"));
	HeaderText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", MainMenuStyle::PanelHeaderFontSize));
	HeaderText->SetColorAndOpacity(FSlateColor(MainMenuStyle::CyanAccentColor));
	HeaderText->SetJustification(ETextJustify::Center);

	if (UVerticalBoxSlot* HeaderSlot = SettingsPanelBox->AddChildToVerticalBox(HeaderText))
	{
		HeaderSlot->SetHorizontalAlignment(HAlign_Center);
		HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
	}

	auto AddSliderRow = [&](const FText& Label, float MinVal, float MaxVal, TObjectPtr<USlider>& OutSlider, TObjectPtr<UTextBlock>& OutValueText, FName RowName)
	{
		UVerticalBox* RowBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), RowName);

		UHorizontalBox* TopLine = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UTextBlock* LabelBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		LabelBlock->SetText(Label);
		LabelBlock->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", MainMenuStyle::LabelFontSize));
		LabelBlock->SetColorAndOpacity(FSlateColor(MainMenuStyle::PrimaryTextColor));

		OutValueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		OutValueText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", MainMenuStyle::LabelFontSize));
		OutValueText->SetColorAndOpacity(FSlateColor(MainMenuStyle::CyanAccentColor));
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
		OutSlider->SetSliderBarColor(MainMenuStyle::ButtonNormalBgColor);
		OutSlider->SetSliderHandleColor(MainMenuStyle::CyanAccentColor);

		if (UVerticalBoxSlot* SliderSlot = RowBox->AddChildToVerticalBox(OutSlider))
		{
			SliderSlot->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 0.0f));
		}

		if (UVerticalBoxSlot* RowSlot = SettingsPanelBox->AddChildToVerticalBox(RowBox))
		{
			RowSlot->SetPadding(FMargin(0.0f, 8.0f));
		}
	};

	// 1. Volume Geral (0 a 1)
	AddSliderRow(LOCTEXT("Setting_Volume", "Volume Geral"), 0.0f, 1.0f, VolumeSlider, VolumeValueText, TEXT("VolumeRow"));

	// 2. Sensibilidade (salva apenas em UMainMenuSaveGame)
	AddSliderRow(LOCTEXT("Setting_Sensitivity", "Sensibilidade"), 0.1f, 5.0f, SensitivitySlider, SensitivityValueText, TEXT("SensitivityRow"));

	auto AddSelectorRow = [&](const FText& Label, TObjectPtr<UButton>& OutPrevBtn, TObjectPtr<UTextBlock>& OutValText, TObjectPtr<UButton>& OutNextBtn, FName RowName)
	{
		UVerticalBox* RowBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), RowName);

		UTextBlock* LabelBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		LabelBlock->SetText(Label);
		LabelBlock->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", MainMenuStyle::LabelFontSize));
		LabelBlock->SetColorAndOpacity(FSlateColor(MainMenuStyle::PrimaryTextColor));

		if (UVerticalBoxSlot* LblSlot = RowBox->AddChildToVerticalBox(LabelBlock))
		{
			LblSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
		}

		UHorizontalBox* SelectorLine = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

		USizeBox* PrevSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		PrevSize->SetMinDesiredWidth(MainMenuStyle::SmallButtonMinWidth);
		PrevSize->SetMinDesiredHeight(40.0f);
		OutPrevBtn = CreateStyledButton(PrevSize, FText::FromString(TEXT("<")), MainMenuStyle::LabelFontSize);
		PrevSize->AddChild(OutPrevBtn);

		OutValText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		OutValText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", MainMenuStyle::LabelFontSize));
		OutValText->SetColorAndOpacity(FSlateColor(MainMenuStyle::SecondaryTextColor));
		OutValText->SetJustification(ETextJustify::Center);

		USizeBox* NextSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		NextSize->SetMinDesiredWidth(MainMenuStyle::SmallButtonMinWidth);
		NextSize->SetMinDesiredHeight(40.0f);
		OutNextBtn = CreateStyledButton(NextSize, FText::FromString(TEXT(">")), MainMenuStyle::LabelFontSize);
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
			RowSlot->SetPadding(FMargin(0.0f, 8.0f));
		}
	};

	// 3. Modo de Janela
	AddSelectorRow(LOCTEXT("Setting_WindowMode", "Modo de Janela"), WindowModePrevButton, WindowModeValueText, WindowModeNextButton, TEXT("WindowModeRow"));

	// 4. Resolucao
	AddSelectorRow(LOCTEXT("Setting_Resolution", "Resolução"), ResolutionPrevButton, ResolutionValueText, ResolutionNextButton, TEXT("ResolutionRow"));

	// TODO: Adicionar secao de rebinding de teclas neste painel em iteracoes futuras.

	// Botoes Aplicar e Voltar
	USizeBox* ApplySizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SettingsApplySizeBox"));
	ApplySizeBox->SetMinDesiredHeight(MainMenuStyle::ButtonMinHeight);
	SettingsApplyButton = CreateStyledButton(ApplySizeBox, LOCTEXT("Btn_Apply", "Aplicar"), MainMenuStyle::ButtonFontSize);
	ApplySizeBox->AddChild(SettingsApplyButton);

	if (UVerticalBoxSlot* ApplySlot = SettingsPanelBox->AddChildToVerticalBox(ApplySizeBox))
	{
		ApplySlot->SetHorizontalAlignment(HAlign_Fill);
		ApplySlot->SetPadding(FMargin(0.0f, 16.0f, 0.0f, MainMenuStyle::ButtonVerticalSpacing));
	}

	USizeBox* BackSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SettingsBackSizeBox"));
	BackSizeBox->SetMinDesiredHeight(MainMenuStyle::ButtonMinHeight);
	SettingsBackButton = CreateStyledButton(BackSizeBox, LOCTEXT("Btn_Back", "Voltar"), MainMenuStyle::ButtonFontSize);
	BackSizeBox->AddChild(SettingsBackButton);

	if (UVerticalBoxSlot* BackSlot = SettingsPanelBox->AddChildToVerticalBox(BackSizeBox))
	{
		BackSlot->SetHorizontalAlignment(HAlign_Fill);
		BackSlot->SetPadding(FMargin(0.0f, MainMenuStyle::ButtonVerticalSpacing));
	}
}

void UMainMenuWidget::BuildCreditsPanel()
{
	UVerticalBox* CreditsPanelBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CreditsPanelBox"));
	ContentSwitcher->AddChild(CreditsPanelBox);

	UTextBlock* HeaderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CreditsHeaderText"));
	HeaderText->SetText(LOCTEXT("Header_Credits", "CRÉDITOS"));
	HeaderText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", MainMenuStyle::PanelHeaderFontSize));
	HeaderText->SetColorAndOpacity(FSlateColor(MainMenuStyle::CyanAccentColor));
	HeaderText->SetJustification(ETextJustify::Center);

	if (UVerticalBoxSlot* HeaderSlot = CreditsPanelBox->AddChildToVerticalBox(HeaderText))
	{
		HeaderSlot->SetHorizontalAlignment(HAlign_Center);
		HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
	}

	UTextBlock* CreditsBody = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CreditsBodyText"));
	CreditsBody->SetText(LOCTEXT(
		"Credits_Body",
		"Desenvolvido por Sonic Studios\n\n"
		"Equipe de Desenvolvimento:\n"
		"Murilo Costa\n"
		"Mateus Favero\n"
		"Gustavo Traldi\n"
		"Bruno Sigrist\n"
		"Derek Santos\n\n"
		"© 2026 Sonic Studios. Todos os direitos reservados.\n"
		"MOMENTUM e todas as marcas associadas são propriedade de Sonic Studios."));
	CreditsBody->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", MainMenuStyle::LabelFontSize));
	CreditsBody->SetColorAndOpacity(FSlateColor(MainMenuStyle::PrimaryTextColor));
	CreditsBody->SetShadowOffset(FVector2D(1.5f, 1.5f));
	CreditsBody->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
	CreditsBody->SetJustification(ETextJustify::Center);
	CreditsBody->SetAutoWrapText(true);

	if (UVerticalBoxSlot* BodySlot = CreditsPanelBox->AddChildToVerticalBox(CreditsBody))
	{
		BodySlot->SetHorizontalAlignment(HAlign_Fill);
		BodySlot->SetPadding(FMargin(8.0f, 12.0f, 8.0f, 24.0f));
	}

	USizeBox* BackSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CreditsBackSizeBox"));
	BackSizeBox->SetMinDesiredHeight(MainMenuStyle::ButtonMinHeight);
	CreditsBackButton = CreateStyledButton(BackSizeBox, LOCTEXT("Btn_Back", "Voltar"), MainMenuStyle::ButtonFontSize);
	BackSizeBox->AddChild(CreditsBackButton);

	if (UVerticalBoxSlot* BackSlot = CreditsPanelBox->AddChildToVerticalBox(BackSizeBox))
	{
		BackSlot->SetHorizontalAlignment(HAlign_Fill);
		BackSlot->SetPadding(FMargin(0.0f, MainMenuStyle::ButtonVerticalSpacing));
	}
}

void UMainMenuWidget::RefreshLevelsList()
{
	if (!LevelsListContainer || !WidgetTree)
	{
		return;
	}

	LevelsListContainer->ClearChildren();
	LevelClickProxies.Empty();
	DynamicLevelButtons.Empty();

	const UMainMenuSettings* MenuSettings = UMainMenuSettings::Get();
	if (!MenuSettings || MenuSettings->Levels.Num() == 0)
	{
		UTextBlock* EmptyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		EmptyText->SetText(LOCTEXT("Levels_Empty", "Nenhuma fase disponível"));
		EmptyText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", MainMenuStyle::LabelFontSize));
		EmptyText->SetColorAndOpacity(FSlateColor(MainMenuStyle::SecondaryTextColor));
		EmptyText->SetJustification(ETextJustify::Center);

		if (UVerticalBoxSlot* EmptySlot = LevelsListContainer->AddChildToVerticalBox(EmptyText))
		{
			EmptySlot->SetHorizontalAlignment(HAlign_Center);
			EmptySlot->SetPadding(FMargin(0.0f, 24.0f));
		}
		return;
	}

	// TODO: Exibir miniaturas das fases ao lado do nome quando adicionadas em FMainMenuLevelEntry.
	for (int32 Index = 0; Index < MenuSettings->Levels.Num(); ++Index)
	{
		const FMainMenuLevelEntry& Entry = MenuSettings->Levels[Index];
		const FText ButtonLabel = Entry.DisplayName.IsEmpty()
			? FText::Format(LOCTEXT("Level_DefaultNameFmt", "Fase {0}"), FText::AsNumber(Index + 1))
			: Entry.DisplayName;

		USizeBox* LevelBtnSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		LevelBtnSize->SetMinDesiredHeight(MainMenuStyle::ButtonMinHeight);

		UButton* LevelBtn = CreateStyledButton(LevelBtnSize, ButtonLabel, MainMenuStyle::ButtonFontSize);
		LevelBtnSize->AddChild(LevelBtn);

		UMainMenuLevelClickProxy* Proxy = NewObject<UMainMenuLevelClickProxy>(this);
		Proxy->Setup(this, Index);
		LevelClickProxies.Add(Proxy);

		LevelBtn->OnClicked.AddDynamic(Proxy, &UMainMenuLevelClickProxy::HandleClicked);
		DynamicLevelButtons.Add(LevelBtn);

		if (UVerticalBoxSlot* ItemSlot = LevelsListContainer->AddChildToVerticalBox(LevelBtnSize))
		{
			ItemSlot->SetHorizontalAlignment(HAlign_Fill);
			ItemSlot->SetPadding(FMargin(0.0f, MainMenuStyle::ButtonVerticalSpacing));
		}
	}
}

void UMainMenuWidget::LoadSettingsToUI()
{
	UMainMenuSaveGame* SaveData = UMainMenuSaveGame::LoadOrCreate();
	if (SaveData)
	{
		CurrentVolume = FMath::Clamp(SaveData->MasterVolume, 0.0f, 1.0f);
		CurrentSensitivity = FMath::Clamp(SaveData->MouseSensitivity, 0.1f, 5.0f);
		CurrentWindowModeIndex = FMath::Clamp(SaveData->PreferredWindowMode, 0, 2);
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

	UpdateSettingsLabels();
}

void UMainMenuWidget::UpdateSettingsLabels()
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
}

void UMainMenuWidget::ApplyVolumeSetting(float VolumeValue)
{
	const float ClampedVolume = FMath::Clamp(VolumeValue, 0.0f, 1.0f);
	if (UWorld* World = GetWorld())
	{
		if (FAudioDeviceHandle AudioDevice = World->GetAudioDevice())
		{
			AudioDevice->SetTransientPrimaryVolume(ClampedVolume);
		}
	}
	// TODO: Integrar SoundMix / SoundClass dedicados (UGameplayStatics::SetSoundMixClassOverride) quando os assets de audio forem criados.
}

void UMainMenuWidget::ShowStatusMessage(const FText& Message, bool bIsWarning)
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

	StatusMessageText->SetColorAndOpacity(FSlateColor(bIsWarning ? MainMenuStyle::WarningTextColor : MainMenuStyle::CyanAccentColor));
	StatusMessageText->SetText(Message);
	StatusMessageText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UMainMenuWidget::SwitchToPanel(int32 PanelIndex, UButton* DefaultFocusButton)
{
	// TODO: Tocar som de UI (clique / mudanca de painel) aqui.
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

void UMainMenuWidget::TransitionToLevel(const TSoftObjectPtr<UWorld>& TargetLevel)
{
	StopBackgroundVideo();

	if (APlayerController* OwningPC = GetOwningPlayer())
	{
		if (AMainMenuPlayerController* MenuPC = Cast<AMainMenuPlayerController>(OwningPC))
		{
			MenuPC->RestoreGameplayInputMode();
		}
		else
		{
			RemoveFromParent();
			FInputModeGameOnly GameInputMode;
			GameInputMode.SetConsumeCaptureMouseDown(false);
			OwningPC->SetInputMode(GameInputMode);
			OwningPC->bShowMouseCursor = false;
			OwningPC->ResetIgnoreInputFlags();
		}
	}

	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetAllUserFocusToGameViewport();
	}

	UGameplayStatics::OpenLevelBySoftObjectPtr(this, TargetLevel);
}

void UMainMenuWidget::OnPlayClicked()
{
	const UMainMenuSettings* MenuSettings = UMainMenuSettings::Get();
	if (!MenuSettings || MenuSettings->PlayLevel.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("[MainMenu] PlayLevel nao esta configurado em Project Settings -> Game -> Main Menu."));
		const FText WarningMsg = LOCTEXT("Msg_PlayLevelEmpty", "Aviso: Nenhum level principal (PlayLevel) configurado em Project Settings > Main Menu.");
		ShowStatusMessage(WarningMsg, true);

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, WarningMsg.ToString());
		}
		return;
	}

	TransitionToLevel(MenuSettings->PlayLevel);
}

void UMainMenuWidget::OnLevelsClicked()
{
	RefreshLevelsList();
	UButton* TargetFocus = DynamicLevelButtons.Num() > 0 ? DynamicLevelButtons[0].Get() : LevelsBackButton.Get();
	SwitchToPanel(PanelIndex_Levels, TargetFocus);
}

void UMainMenuWidget::OnSettingsClicked()
{
	LoadSettingsToUI();
	SwitchToPanel(PanelIndex_Settings, SettingsApplyButton.Get());
}

void UMainMenuWidget::OnCreditsClicked()
{
	SwitchToPanel(PanelIndex_Credits, CreditsBackButton.Get());
}

void UMainMenuWidget::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UMainMenuWidget::OnBackToMainClicked()
{
	SwitchToPanel(PanelIndex_Main, PlayButton.Get());
}

void UMainMenuWidget::OpenLevelByIndex(int32 Index)
{
	const UMainMenuSettings* MenuSettings = UMainMenuSettings::Get();
	if (!MenuSettings || !MenuSettings->Levels.IsValidIndex(Index))
	{
		UE_LOG(LogTemp, Warning, TEXT("[MainMenu] Indice de fase invalido: %d"), Index);
		return;
	}

	const FMainMenuLevelEntry& Entry = MenuSettings->Levels[Index];
	if (Entry.Level.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("[MainMenu] A fase no indice %d ('%s') nao possui um Level configurado."), Index, *Entry.DisplayName.ToString());
		const FText WarningMsg = LOCTEXT("Msg_EntryLevelEmpty", "Aviso: Esta fase não possui um mapa associado nas configurações.");
		ShowStatusMessage(WarningMsg, true);

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, WarningMsg.ToString());
		}
		return;
	}

	TransitionToLevel(Entry.Level);
}

void UMainMenuWidget::OnVolumeChanged(float NewValue)
{
	CurrentVolume = FMath::Clamp(NewValue, 0.0f, 1.0f);
	UpdateSettingsLabels();
}

void UMainMenuWidget::OnSensitivityChanged(float NewValue)
{
	CurrentSensitivity = FMath::Clamp(NewValue, 0.1f, 5.0f);
	UpdateSettingsLabels();
}

void UMainMenuWidget::OnWindowModePrevClicked()
{
	CurrentWindowModeIndex = (CurrentWindowModeIndex + 2) % 3;
	UpdateSettingsLabels();
}

void UMainMenuWidget::OnWindowModeNextClicked()
{
	CurrentWindowModeIndex = (CurrentWindowModeIndex + 1) % 3;
	UpdateSettingsLabels();
}

void UMainMenuWidget::OnResolutionPrevClicked()
{
	if (AvailableResolutions.Num() > 0)
	{
		CurrentResolutionIndex = (CurrentResolutionIndex - 1 + AvailableResolutions.Num()) % AvailableResolutions.Num();
		UpdateSettingsLabels();
	}
}

void UMainMenuWidget::OnResolutionNextClicked()
{
	if (AvailableResolutions.Num() > 0)
	{
		CurrentResolutionIndex = (CurrentResolutionIndex + 1) % AvailableResolutions.Num();
		UpdateSettingsLabels();
	}
}

void UMainMenuWidget::OnSettingsApplyClicked()
{
	ApplyVolumeSetting(CurrentVolume);

	const FIntPoint SelectedRes = AvailableResolutions.IsValidIndex(CurrentResolutionIndex)
		? AvailableResolutions[CurrentResolutionIndex]
		: FIntPoint(1920, 1080);

	if (UGameUserSettings* UserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
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
		UserSettings->ApplySettings(false);
	}

	if (UMainMenuSaveGame* SaveData = UMainMenuSaveGame::LoadOrCreate())
	{
		SaveData->MasterVolume = CurrentVolume;
		SaveData->MouseSensitivity = CurrentSensitivity;
		SaveData->PreferredWindowMode = CurrentWindowModeIndex;
		SaveData->PreferredResolution = SelectedRes;
		SaveData->SaveToSlot();
	}

	UMainMenuSaveGame::ApplySavedPreferencesToWorld(GetWorld());

	ShowStatusMessage(LOCTEXT("Msg_SettingsSaved", "Configurações aplicadas e salvas com sucesso!"), false);
}

bool UMainMenuWidget::StartBackgroundVideo()
{
	if (!BackgroundImage)
	{
		return false;
	}

	// Localiza o arquivo de video empacotado no projeto (Content/MainMenu/Movies/main_menu_v1.mp4) ou no caminho original
	TArray<FString> CandidatePaths = {
		FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("MainMenu/Movies/main_menu_v1.mp4")),
		FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Movies/main_menu_v1.mp4")),
		TEXT("C:/Users/52318488/Downloads/main_menu_v1.mp4")
	};

	FString ResolvedVideoPath;
	for (const FString& Candidate : CandidatePaths)
	{
		if (FPaths::FileExists(Candidate))
		{
			ResolvedVideoPath = Candidate;
			break;
		}
	}

	if (ResolvedVideoPath.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[MainMenu] Arquivo de video main_menu_v1.mp4 nao encontrado. Mantendo gradiente de fundo."));
		return false;
	}

	StopBackgroundVideo();

	BackgroundMediaPlayer = NewObject<UMediaPlayer>(this, TEXT("MainMenuBackgroundMediaPlayer"));
	BackgroundMediaPlayer->PlayOnOpen = true;
	BackgroundMediaPlayer->SetLooping(true);
	BackgroundMediaPlayer->OnMediaOpened.AddDynamic(this, &UMainMenuWidget::OnBackgroundMediaOpened);
	BackgroundMediaPlayer->OnEndReached.AddDynamic(this, &UMainMenuWidget::OnBackgroundMediaEndReached);

	BackgroundMediaTexture = NewObject<UMediaTexture>(this, TEXT("MainMenuBackgroundMediaTexture"));
	BackgroundMediaTexture->AddressX = TA_Clamp;
	BackgroundMediaTexture->AddressY = TA_Clamp;
	BackgroundMediaTexture->AutoClear = true;
	BackgroundMediaTexture->ClearColor = MainMenuStyle::GradientTopColor;
	BackgroundMediaTexture->NewStyleOutput = true;
	BackgroundMediaTexture->SetMediaPlayer(BackgroundMediaPlayer);
	BackgroundMediaTexture->UpdateResource();

	BackgroundMediaSource = NewObject<UFileMediaSource>(this, TEXT("MainMenuBackgroundMediaSource"));
	BackgroundMediaSource->SetFilePath(ResolvedVideoPath);

	FSlateBrush VideoBrush;
	VideoBrush.DrawAs = ESlateBrushDrawType::Image;
	VideoBrush.SetResourceObject(BackgroundMediaTexture);
	VideoBrush.ImageSize = FVector2D(1920.0f, 1080.0f);
	BackgroundImage->SetBrush(VideoBrush);
	BackgroundImage->SetColorAndOpacity(FLinearColor::White);

	const bool bOpened = BackgroundMediaPlayer->OpenSource(BackgroundMediaSource);
	if (!bOpened)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MainMenu] Falha ao abrir o video de fundo: %s"), *ResolvedVideoPath);
		if (BackgroundGradientTexture)
		{
			BackgroundImage->SetBrushFromTexture(BackgroundGradientTexture, false);
		}
		return false;
	}

	return true;
}

void UMainMenuWidget::StopBackgroundVideo()
{
	if (BackgroundMediaPlayer)
	{
		BackgroundMediaPlayer->OnMediaOpened.RemoveAll(this);
		BackgroundMediaPlayer->OnEndReached.RemoveAll(this);
		BackgroundMediaPlayer->Close();
		BackgroundMediaPlayer = nullptr;
	}

	if (BackgroundMediaTexture)
	{
		BackgroundMediaTexture->SetMediaPlayer(nullptr);
		BackgroundMediaTexture = nullptr;
	}

	BackgroundMediaSource = nullptr;
}

void UMainMenuWidget::OnBackgroundMediaOpened(FString OpenedUrl)
{
	if (BackgroundMediaPlayer)
	{
		BackgroundMediaPlayer->SetLooping(true);
		BackgroundMediaPlayer->Play();
	}
}

void UMainMenuWidget::OnBackgroundMediaEndReached()
{
	if (BackgroundMediaPlayer && !BackgroundMediaPlayer->IsPlaying())
	{
		BackgroundMediaPlayer->Rewind();
		BackgroundMediaPlayer->Play();
	}
}

UButton* UMainMenuWidget::CreateStyledButton(UWidget* Outer, const FText& LabelText, int32 FontSize)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());

	auto MakeRoundedBrush = [](const FLinearColor& FillColor) -> FSlateBrush
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(FillColor);
		Brush.OutlineSettings.CornerRadii = FVector4(
			MainMenuStyle::ButtonCornerRadius,
			MainMenuStyle::ButtonCornerRadius,
			MainMenuStyle::ButtonCornerRadius,
			MainMenuStyle::ButtonCornerRadius);
		Brush.OutlineSettings.Color = FSlateColor(MainMenuStyle::ButtonBorderCyanColor);
		Brush.OutlineSettings.Width = MainMenuStyle::ButtonBorderThickness;
		Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		Brush.OutlineSettings.bUseBrushTransparency = true;
		return Brush;
	};

	FButtonStyle BtnStyle = Button->GetStyle();
	BtnStyle.SetNormal(MakeRoundedBrush(MainMenuStyle::ButtonNormalBgColor));
	BtnStyle.SetHovered(MakeRoundedBrush(MainMenuStyle::ButtonHoverBgColor));
	BtnStyle.SetPressed(MakeRoundedBrush(MainMenuStyle::ButtonPressedBgColor));
	BtnStyle.SetNormalPadding(FMargin(16.0f, 10.0f));
	BtnStyle.SetPressedPadding(FMargin(16.0f, 11.0f, 16.0f, 9.0f));
	Button->SetStyle(BtnStyle);

	UTextBlock* BtnLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	BtnLabel->SetText(LabelText);
	BtnLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", FontSize));
	BtnLabel->SetColorAndOpacity(FSlateColor(MainMenuStyle::PrimaryTextColor));
	BtnLabel->SetJustification(ETextJustify::Center);

	if (UButtonSlot* BtnSlot = Cast<UButtonSlot>(Button->AddChild(BtnLabel)))
	{
		BtnSlot->SetHorizontalAlignment(HAlign_Center);
		BtnSlot->SetVerticalAlignment(VAlign_Center);
	}

	return Button;
}

UTexture2D* UMainMenuWidget::CreateProceduralGradientTexture()
{
	constexpr int32 Width = 4;
	constexpr int32 Height = 256;

	UTexture2D* GradientTex = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
	if (!GradientTex || !GradientTex->GetPlatformData() || GradientTex->GetPlatformData()->Mips.Num() == 0)
	{
		return nullptr;
	}

	GradientTex->SRGB = true;
	GradientTex->AddressX = TA_Clamp;
	GradientTex->AddressY = TA_Clamp;

	FTexture2DMipMap& Mip = GradientTex->GetPlatformData()->Mips[0];
	void* RawData = Mip.BulkData.Lock(LOCK_READ_WRITE);
	if (RawData)
	{
		FColor* Pixels = static_cast<FColor*>(RawData);
		for (int32 Y = 0; Y < Height; ++Y)
		{
			const float Alpha = static_cast<float>(Y) / static_cast<float>(Height - 1);
			const FLinearColor RowLinear = FMath::Lerp(MainMenuStyle::GradientTopColor, MainMenuStyle::GradientBottomColor, Alpha);
			const FColor RowColor = RowLinear.ToFColor(true);

			for (int32 X = 0; X < Width; ++X)
			{
				Pixels[Y * Width + X] = RowColor;
			}
		}
		Mip.BulkData.Unlock();
		GradientTex->UpdateResource();
	}

	return GradientTex;
}

UTexture2D* UMainMenuWidget::LoadTrimmedTitleLogoTexture(FVector2D& OutDisplaySize)
{
	TArray<FString> CandidatePaths = {
		FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("MainMenu/Textures/titulo_momentum.png")),
		TEXT("C:/Users/52318488/Downloads/titulo_momentum.png")
	};

	FString ResolvedLogoPath;
	for (const FString& Candidate : CandidatePaths)
	{
		if (FPaths::FileExists(Candidate))
		{
			ResolvedLogoPath = Candidate;
			break;
		}
	}

	if (ResolvedLogoPath.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[MainMenu] Arquivo de logo titulo_momentum.png nao encontrado."));
		return nullptr;
	}

	FImage LoadedImage;
	if (!FImageUtils::LoadImage(*ResolvedLogoPath, LoadedImage))
	{
		UE_LOG(LogTemp, Warning, TEXT("[MainMenu] Falha ao carregar imagem PNG do titulo: %s"), *ResolvedLogoPath);
		return nullptr;
	}

	FImage BgraImage;
	LoadedImage.CopyTo(BgraImage, ERawImageFormat::BGRA8, EGammaSpace::sRGB);

	const int32 FullWidth = BgraImage.SizeX;
	const int32 FullHeight = BgraImage.SizeY;
	if (FullWidth <= 0 || FullHeight <= 0)
	{
		return nullptr;
	}

	TArrayView64<const FColor> SrcPixels = BgraImage.AsBGRA8();

	// Recorta as bordas transparentes da imagem para exibir o logo na proporcao exata sem espaco vazio excessivo
	int32 MinX = FullWidth;
	int32 MinY = FullHeight;
	int32 MaxX = 0;
	int32 MaxY = 0;

	for (int32 Y = 0; Y < FullHeight; ++Y)
	{
		for (int32 X = 0; X < FullWidth; ++X)
		{
			const FColor& Pixel = SrcPixels[static_cast<int64>(Y) * FullWidth + X];
			if (Pixel.A > 4)
			{
				MinX = FMath::Min(MinX, X);
				MinY = FMath::Min(MinY, Y);
				MaxX = FMath::Max(MaxX, X);
				MaxY = FMath::Max(MaxY, Y);
			}
		}
	}

	if (MinX > MaxX || MinY > MaxY)
	{
		MinX = 0;
		MinY = 0;
		MaxX = FullWidth - 1;
		MaxY = FullHeight - 1;
	}
	else
	{
		constexpr int32 CropMargin = 8;
		MinX = FMath::Max(0, MinX - CropMargin);
		MinY = FMath::Max(0, MinY - CropMargin);
		MaxX = FMath::Min(FullWidth - 1, MaxX + CropMargin);
		MaxY = FMath::Min(FullHeight - 1, MaxY + CropMargin);
	}

	const int32 CroppedWidth = MaxX - MinX + 1;
	const int32 CroppedHeight = MaxY - MinY + 1;

	UTexture2D* LogoTex = UTexture2D::CreateTransient(CroppedWidth, CroppedHeight, PF_B8G8R8A8);
	if (!LogoTex || !LogoTex->GetPlatformData() || LogoTex->GetPlatformData()->Mips.Num() == 0)
	{
		return nullptr;
	}

	LogoTex->SRGB = true;
	LogoTex->Filter = TF_Bilinear;
	LogoTex->AddressX = TA_Clamp;
	LogoTex->AddressY = TA_Clamp;

	FTexture2DMipMap& Mip = LogoTex->GetPlatformData()->Mips[0];
	void* RawData = Mip.BulkData.Lock(LOCK_READ_WRITE);
	if (RawData)
	{
		FColor* DstPixels = static_cast<FColor*>(RawData);
		for (int32 Y = 0; Y < CroppedHeight; ++Y)
		{
			const int64 SrcRowOffset = static_cast<int64>(MinY + Y) * FullWidth + MinX;
			const int64 DstRowOffset = static_cast<int64>(Y) * CroppedWidth;
			FMemory::Memcpy(DstPixels + DstRowOffset, SrcPixels.GetData() + SrcRowOffset, CroppedWidth * sizeof(FColor));
		}
		Mip.BulkData.Unlock();
		LogoTex->UpdateResource();
	}

	const float AspectRatio = static_cast<float>(CroppedHeight) / static_cast<float>(FMath::Max(1, CroppedWidth));
	OutDisplaySize = FVector2D(MainMenuStyle::TitleLogoDisplayWidth, MainMenuStyle::TitleLogoDisplayWidth * AspectRatio);

	return LogoTex;
}

#undef LOCTEXT_NAMESPACE
