#include "MainMenuWidget.h"
#include "MainMenuSettings.h"
#include "MainMenuSaveGame.h"
#include "MainMenuPlayerController.h"
#include "SurfGameInstance.h"
#include "Animation/WidgetAnimation.h"
#include "MovieScene.h"
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
#include "Components/ScrollBox.h"
#include "Components/Border.h"
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

#define LOCTEXT_NAMESPACE "MainMenu"

namespace MainMenuStyle
{
	// Conversao sRGB Hex -> Linear precisa para fidelidade cromatica 1:1
	static FLinearColor HexToLinear(const TCHAR* HexString, float Alpha = 1.0f)
	{
		FLinearColor Color = FLinearColor::FromSRGBColor(FColor::FromHex(HexString));
		Color.A = Alpha;
		return Color;
	}

	// Paleta e Estados Visuais
	// Titulo principal "MOMENTUM": Marrom-escuro solido (#2B1B1A) com alto contraste e Drop Shadow sutil
	static const FLinearColor TitleDarkBrownColor = HexToLinear(TEXT("2B1B1A"), 1.0f);

	// Botoes Paralelogramo:
	// - Normal: Marrom/Cinza escuro solido (#2E1E1C) com opacidade 0.95
	// - Hovered: Destaque vibrante/claro (#523632) com leve deslocamento horizontal (+8px)
	// - Pressed: Tom mais escuro (#1A1110)
	static const FLinearColor ButtonNormalBgColor = HexToLinear(TEXT("2E1E1C"), 0.95f);
	static const FLinearColor ButtonHoverBgColor = HexToLinear(TEXT("523632"), 0.98f);
	static const FLinearColor ButtonPressedBgColor = HexToLinear(TEXT("1A1110"), 1.0f);
	static const FLinearColor ButtonSubtleBorderColor = HexToLinear(TEXT("3A2624"), 0.85f);

	// Cor do Texto Interno: Creme/Branco suave (#F3E3D3)
	static const FLinearColor CreamTextColor = HexToLinear(TEXT("F3E3D3"), 1.0f);
	static const FLinearColor PeachAccentColor = HexToLinear(TEXT("E2BA9D"), 1.0f);
	static const FLinearColor WarmSecondaryTextColor = HexToLinear(TEXT("C89578"), 1.0f);
	static const FLinearColor SubmenuCardBgColor = HexToLinear(TEXT("2A1719"), 0.94f);

	// Inclinacao diagonal estilo paralelogramo:
	// Render Transform -> Shear X = -0.22 (atan(-0.22) = -12.407418 graus em FWidgetTransform::Shear)
	// e Shear X = +0.22 (+12.407418 graus) no TextBlock interno para manter as letras retas.
	static constexpr float ButtonShearSlopeX = -0.22f;
	static constexpr float ButtonShearAngleDeg = -12.407418f;

	// Deslocamento horizontal no estado Hovered: Translation X = +8px
	static constexpr float ButtonHoverTranslationX = 8.0f;
	static constexpr float ButtonHoverInterpSpeed = 18.0f;

	// Parametros da Transicao Continua (Anim_StartGame)
	static constexpr float InitialBackgroundBlurStrength = 25.0f;
	static constexpr float StartGameAnimDuration = 0.60f;
	static constexpr float StartGameContainerFadeDuration = 0.35f;

	// Dimensoes e espacamentos do layout
	static constexpr float LeftColumnOffsetX = 100.0f; // Margem esquerda de 100px
	static constexpr float MainButtonWidth = 310.0f;
	static constexpr float SubmenuPanelWidth = 420.0f;
	static constexpr float ButtonMinHeight = 50.0f;
	static constexpr float SmallButtonMinWidth = 48.0f;
	static constexpr float ButtonCornerRadius = 2.0f;
	static constexpr float ButtonBorderThickness = 1.0f;
	static constexpr float ButtonVerticalPadding = 10.0f; // Espacamento vertical (Padding) de 10px entre botoes
	static constexpr float LevelsScrollMaxHeight = 260.0f;
	static constexpr float DiscordButtonSize = 46.0f;

	// Tamanhos de fonte e tracking
	static constexpr int32 TitleFontSize = 68;
	static constexpr int32 TitleLetterSpacing = 280;
	static constexpr int32 PanelHeaderFontSize = 22;
	static constexpr int32 ButtonFontSize = 18;
	static constexpr int32 ButtonLetterSpacing = 50;
	static constexpr int32 LabelFontSize = 14;
	static constexpr int32 StatusFontSize = 13;
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

	AllStyledButtons.Empty();

	// 1. Raiz (Canvas Panel) 100% transparente para revelar o Mapa 1 em tempo real atras da UI
	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	RootCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = RootCanvas;

	// 2. BackgroundBlur_Main (Z-Order: 0): Full Fill (0,0 a 1,1), Offsets zerados (0.0), Blur Strength = 25.0
	BackgroundBlur_Main = WidgetTree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass(), TEXT("BackgroundBlur_Main"));
	BackgroundBlur_Main->SetBlurStrength(MainMenuStyle::InitialBackgroundBlurStrength);
	BackgroundBlur_Main->SetApplyAlphaToBlur(true);
	BackgroundBlur_Main->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (UCanvasPanelSlot* BlurSlot = RootCanvas->AddChildToCanvas(BackgroundBlur_Main))
	{
		BlurSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		BlurSlot->SetOffsets(FMargin(0.0f, 0.0f, 0.0f, 0.0f));
		BlurSlot->SetZOrder(0);
	}

	// 3. Container_UI (Canvas Panel - Z-Order: 1): agrupa todos os elementos visuais e interativos
	Container_UI = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Container_UI"));
	Container_UI->SetRenderOpacity(1.0f);
	Container_UI->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (UCanvasPanelSlot* ContainerSlot = RootCanvas->AddChildToCanvas(Container_UI))
	{
		ContainerSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		ContainerSlot->SetOffsets(FMargin(0.0f, 0.0f, 0.0f, 0.0f));
		ContainerSlot->SetZOrder(1);
	}

	// 4. Lado Direito / Centro-Direito (Logo): TextBlock "MOMENTUM" (#2B1B1A, Drop Shadow sutil, LetterSpacing amplo)
	TitleTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleTextBlock"));
	TitleTextBlock->SetText(LOCTEXT("Title_Momentum", "MOMENTUM"));

	FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", MainMenuStyle::TitleFontSize);
	TitleFont.LetterSpacing = MainMenuStyle::TitleLetterSpacing;
	TitleTextBlock->SetFont(TitleFont);
	TitleTextBlock->SetColorAndOpacity(FSlateColor(MainMenuStyle::TitleDarkBrownColor));
	TitleTextBlock->SetShadowOffset(FVector2D(2.0f, 2.0f));
	TitleTextBlock->SetShadowColorAndOpacity(FLinearColor(0.95f, 0.89f, 0.82f, 0.45f));
	TitleTextBlock->SetJustification(ETextJustify::Center);
	TitleTextBlock->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (UCanvasPanelSlot* TitleSlot = Container_UI->AddChildToCanvas(TitleTextBlock))
	{
		TitleSlot->SetAnchors(FAnchors(0.64f, 0.47f, 0.64f, 0.47f));
		TitleSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		TitleSlot->SetPosition(FVector2D(0.0f, 0.0f));
		TitleSlot->SetAutoSize(true);
		TitleSlot->SetZOrder(1);
	}

	// 5. Lado Esquerdo (Lista de Botoes): Vertical Box ancorada em Left-Center (Anchor X: 0.0, Y: 0.5, Alignment X: 0.0, Y: 0.5)
	//    posicionada com margem esquerda de 100px
	UVerticalBox* LeftColumnLayout = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LeftColumnLayout"));
	if (UCanvasPanelSlot* LeftSlot = Container_UI->AddChildToCanvas(LeftColumnLayout))
	{
		LeftSlot->SetAnchors(FAnchors(0.0f, 0.5f, 0.0f, 0.5f));
		LeftSlot->SetAlignment(FVector2D(0.0f, 0.5f));
		LeftSlot->SetPosition(FVector2D(MainMenuStyle::LeftColumnOffsetX, 0.0f));
		LeftSlot->SetAutoSize(true);
		LeftSlot->SetZOrder(2);
	}

	// Switcher de paineis (0: Menu Principal, 1: Fases, 2: Configuracoes, 3: Creditos)
	ContentSwitcher = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("ContentSwitcher"));
	if (UVerticalBoxSlot* SwitcherSlot = LeftColumnLayout->AddChildToVerticalBox(ContentSwitcher))
	{
		SwitcherSlot->SetHorizontalAlignment(HAlign_Left);
		SwitcherSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	// Mensagem de status/aviso abaixo da coluna esquerda
	StatusMessageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusMessageText"));
	StatusMessageText->SetText(FText::GetEmpty());
	StatusMessageText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", MainMenuStyle::StatusFontSize));
	StatusMessageText->SetColorAndOpacity(FSlateColor(MainMenuStyle::CreamTextColor));
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

	BuildMainPanel();
	BuildLevelsPanel();
	BuildSettingsPanel();
	BuildCreditsPanel();
	BuildStartGameWidgetAnimation();

	ContentSwitcher->SetActiveWidgetIndex(PanelIndex_Main);

	return Super::RebuildWidget();
}

void UMainMenuWidget::BuildStartGameWidgetAnimation()
{
	if (Anim_StartGame)
	{
		return;
	}

	Anim_StartGame = NewObject<UWidgetAnimation>(this, TEXT("Anim_StartGame"));
	if (Anim_StartGame)
	{
		UMovieScene* MovieScene = NewObject<UMovieScene>(Anim_StartGame, TEXT("Anim_StartGame_MovieScene"));
		if (MovieScene)
		{
			const FFrameRate TickResolution = MovieScene->GetTickResolution();
			const FFrameNumber EndFrame = (MainMenuStyle::StartGameAnimDuration * TickResolution).RoundToFrame();
			MovieScene->SetPlaybackRange(TRange<FFrameNumber>(0, EndFrame.Value));
			Anim_StartGame->MovieScene = MovieScene;
		}
	}
}

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	bIsPlayingStartTransition = false;
	StartTransitionElapsed = 0.0f;

	if (BackgroundBlur_Main)
	{
		BackgroundBlur_Main->SetBlurStrength(MainMenuStyle::InitialBackgroundBlurStrength);
	}
	if (Container_UI)
	{
		Container_UI->SetRenderOpacity(1.0f);
		Container_UI->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

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
	if (DiscordButton)
	{
		DiscordButton->OnClicked.RemoveAll(this);
		DiscordButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnDiscordClicked);
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
	if (FOVSlider)
	{
		FOVSlider->OnValueChanged.RemoveAll(this);
		FOVSlider->OnValueChanged.AddDynamic(this, &UMainMenuWidget::OnFOVChanged);
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

	LoadSettingsToUI();
	ApplyVolumeSetting(CurrentVolume);
	RefreshLevelsList();
	FocusInitialButton();
}

void UMainMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 1. Animacao de Hover nos botoes estilizados: deslocamento horizontal suave (Translation X: +8px)
	for (UButton* Btn : AllStyledButtons)
	{
		if (!Btn)
		{
			continue;
		}

		const bool bHighlighted = Btn->IsHovered() || Btn->HasKeyboardFocus();
		const float TargetX = bHighlighted ? MainMenuStyle::ButtonHoverTranslationX : 0.0f;
		const float CurrentX = Btn->GetRenderTransform().Translation.X;
		const float NewX = FMath::FInterpTo(CurrentX, TargetX, InDeltaTime, MainMenuStyle::ButtonHoverInterpSpeed);
		if (!FMath::IsNearlyEqual(CurrentX, NewX, 0.05f))
		{
			Btn->SetRenderTranslation(FVector2D(NewX, 0.0f));
		}
	}

	// 2. Execucao das 2 Tracks da Widget Animation "Anim_StartGame" (0.60s) com interpolacao Cubic Ease Out:
	//    - Track 1 (BackgroundBlur_Main -> Blur Strength): 0.00s (25.0) -> 0.60s (0.0)
	//    - Track 2 (Container_UI -> Render Opacity): 0.00s (1.0) -> 0.35s (0.0)
	if (bIsPlayingStartTransition)
	{
		StartTransitionElapsed += InDeltaTime;

		if (BackgroundBlur_Main)
		{
			const float BlurAlpha = FMath::Clamp(StartTransitionElapsed / MainMenuStyle::StartGameAnimDuration, 0.0f, 1.0f);
			const float BlurValue = FMath::InterpEaseOut(MainMenuStyle::InitialBackgroundBlurStrength, 0.0f, BlurAlpha, 3.0f);
			BackgroundBlur_Main->SetBlurStrength(BlurValue);
		}

		if (Container_UI)
		{
			const float OpacityAlpha = FMath::Clamp(StartTransitionElapsed / MainMenuStyle::StartGameContainerFadeDuration, 0.0f, 1.0f);
			const float OpacityValue = FMath::InterpEaseOut(1.0f, 0.0f, OpacityAlpha, 3.0f);
			Container_UI->SetRenderOpacity(OpacityValue);
		}

		if (StartTransitionElapsed >= MainMenuStyle::StartGameAnimDuration)
		{
			OnStartGameAnimationFinished();
		}
	}
}

FReply UMainMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (bIsPlayingStartTransition)
	{
		return FReply::Handled();
	}

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
		BtnSizeBox->SetWidthOverride(MainMenuStyle::MainButtonWidth);
		BtnSizeBox->SetMinDesiredHeight(MainMenuStyle::ButtonMinHeight);

		UButton* Btn = CreateStyledButton(BtnSizeBox, Label, MainMenuStyle::ButtonFontSize);
		BtnSizeBox->AddChild(Btn);

		if (UVerticalBoxSlot* BoxSlot = MainBox->AddChildToVerticalBox(BtnSizeBox))
		{
			BoxSlot->SetHorizontalAlignment(HAlign_Left);
			// Espacamento vertical (Padding) de 10px entre botoes
			BoxSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, MainMenuStyle::ButtonVerticalPadding));
		}
		return Btn;
	};

	// Ordem vertical estrita: 1. Jogar, 2. Fases, 3. Configuracoes, 4. Creditos, 5. Sair
	PlayButton = AddMenuButton(LOCTEXT("Btn_Play", "Jogar"), TEXT("PlayBtnSizeBox"));
	LevelsButton = AddMenuButton(LOCTEXT("Btn_Levels", "Fases"), TEXT("LevelsBtnSizeBox"));
	SettingsButton = AddMenuButton(LOCTEXT("Btn_Settings", "Configurações"), TEXT("SettingsBtnSizeBox"));
	CreditsButton = AddMenuButton(LOCTEXT("Btn_Credits", "Créditos"), TEXT("CreditsBtnSizeBox"));
	QuitButton = AddMenuButton(LOCTEXT("Btn_Quit", "Sair"), TEXT("QuitBtnSizeBox"));

	// Icone do Discord alinhado abaixo da lista de botoes (100% procedural, sem PNG externo)
	USizeBox* DiscordSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DiscordBtnSizeBox"));
	DiscordSizeBox->SetWidthOverride(MainMenuStyle::DiscordButtonSize);
	DiscordSizeBox->SetHeightOverride(MainMenuStyle::DiscordButtonSize);

	DiscordButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DiscordButton"));
	auto MakeDiscordBrush = [](const FLinearColor& FillColor) -> FSlateBrush
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(FillColor);
		Brush.OutlineSettings.CornerRadii = FVector4(8.0f, 8.0f, 8.0f, 8.0f);
		Brush.OutlineSettings.Color = FSlateColor(MainMenuStyle::ButtonSubtleBorderColor);
		Brush.OutlineSettings.Width = 1.0f;
		Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		Brush.OutlineSettings.bUseBrushTransparency = true;
		return Brush;
	};

	FButtonStyle DiscordStyle = DiscordButton->GetStyle();
	DiscordStyle.SetNormal(MakeDiscordBrush(MainMenuStyle::ButtonNormalBgColor));
	DiscordStyle.SetHovered(MakeDiscordBrush(MainMenuStyle::ButtonHoverBgColor));
	DiscordStyle.SetPressed(MakeDiscordBrush(MainMenuStyle::ButtonPressedBgColor));
	DiscordStyle.SetNormalPadding(FMargin(8.0f));
	DiscordStyle.SetPressedPadding(FMargin(8.0f, 9.0f, 8.0f, 7.0f));
	DiscordButton->SetStyle(DiscordStyle);

	DiscordIconTexture = CreateProceduralDiscordIconTexture();
	UImage* DiscordIconImg = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("DiscordIconImage"));
	if (DiscordIconTexture)
	{
		FSlateBrush IconBrush;
		IconBrush.DrawAs = ESlateBrushDrawType::Image;
		IconBrush.SetResourceObject(DiscordIconTexture);
		IconBrush.ImageSize = FVector2D(28.0f, 28.0f);
		DiscordIconImg->SetBrush(IconBrush);
	}
	DiscordIconImg->SetColorAndOpacity(MainMenuStyle::CreamTextColor);

	if (UButtonSlot* IconSlot = Cast<UButtonSlot>(DiscordButton->AddChild(DiscordIconImg)))
	{
		IconSlot->SetHorizontalAlignment(HAlign_Center);
		IconSlot->SetVerticalAlignment(VAlign_Center);
	}

	DiscordSizeBox->AddChild(DiscordButton);

	if (UVerticalBoxSlot* DiscordSlot = MainBox->AddChildToVerticalBox(DiscordSizeBox))
	{
		DiscordSlot->SetHorizontalAlignment(HAlign_Left);
		DiscordSlot->SetPadding(FMargin(4.0f, 12.0f, 0.0f, 0.0f));
	}
}

UBorder* UMainMenuWidget::CreateSubmenuCardContainer(FName CardName)
{
	UBorder* CardBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), CardName);

	FSlateBrush CardBrush;
	CardBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
	CardBrush.TintColor = FSlateColor(MainMenuStyle::SubmenuCardBgColor);
	CardBrush.OutlineSettings.CornerRadii = FVector4(4.0f, 4.0f, 4.0f, 4.0f);
	CardBrush.OutlineSettings.Color = FSlateColor(MainMenuStyle::ButtonSubtleBorderColor);
	CardBrush.OutlineSettings.Width = 1.5f;
	CardBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
	CardBrush.OutlineSettings.bUseBrushTransparency = true;

	CardBorder->SetBrush(CardBrush);
	CardBorder->SetPadding(FMargin(24.0f, 20.0f));
	return CardBorder;
}

void UMainMenuWidget::BuildLevelsPanel()
{
	USizeBox* CardSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LevelsCardSizeBox"));
	CardSizeBox->SetWidthOverride(MainMenuStyle::SubmenuPanelWidth);
	ContentSwitcher->AddChild(CardSizeBox);

	UBorder* CardBorder = CreateSubmenuCardContainer(TEXT("LevelsCardBorder"));
	CardSizeBox->AddChild(CardBorder);

	UVerticalBox* LevelsPanelBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LevelsPanelBox"));
	CardBorder->AddChild(LevelsPanelBox);

	UTextBlock* HeaderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LevelsHeaderText"));
	HeaderText->SetText(LOCTEXT("Header_Levels", "SELECIONAR FASE"));
	HeaderText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", MainMenuStyle::PanelHeaderFontSize));
	HeaderText->SetColorAndOpacity(FSlateColor(MainMenuStyle::PeachAccentColor));
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
		ListSlot->SetPadding(FMargin(8.0f, 0.0f, 8.0f, 16.0f));
	}

	USizeBox* BackSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LevelsBackSizeBox"));
	BackSizeBox->SetMinDesiredHeight(MainMenuStyle::ButtonMinHeight);
	LevelsBackButton = CreateStyledButton(BackSizeBox, LOCTEXT("Btn_Back", "Voltar"), MainMenuStyle::ButtonFontSize);
	BackSizeBox->AddChild(LevelsBackButton);

	if (UVerticalBoxSlot* BackSlot = LevelsPanelBox->AddChildToVerticalBox(BackSizeBox))
	{
		BackSlot->SetHorizontalAlignment(HAlign_Fill);
		BackSlot->SetPadding(FMargin(8.0f, 4.0f));
	}
}

void UMainMenuWidget::BuildSettingsPanel()
{
	USizeBox* CardSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SettingsCardSizeBox"));
	CardSizeBox->SetWidthOverride(MainMenuStyle::SubmenuPanelWidth);
	ContentSwitcher->AddChild(CardSizeBox);

	UBorder* CardBorder = CreateSubmenuCardContainer(TEXT("SettingsCardBorder"));
	CardSizeBox->AddChild(CardBorder);

	UVerticalBox* SettingsPanelBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsPanelBox"));
	CardBorder->AddChild(SettingsPanelBox);

	UTextBlock* HeaderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SettingsHeaderText"));
	HeaderText->SetText(LOCTEXT("Header_Settings", "CONFIGURAÇÕES"));
	HeaderText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", MainMenuStyle::PanelHeaderFontSize));
	HeaderText->SetColorAndOpacity(FSlateColor(MainMenuStyle::PeachAccentColor));
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
		LabelBlock->SetColorAndOpacity(FSlateColor(MainMenuStyle::CreamTextColor));

		OutValueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		OutValueText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", MainMenuStyle::LabelFontSize));
		OutValueText->SetColorAndOpacity(FSlateColor(MainMenuStyle::PeachAccentColor));
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
		OutSlider->SetSliderHandleColor(MainMenuStyle::PeachAccentColor);

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

	// 2. Sensibilidade (salva em UMainMenuSaveGame)
	AddSliderRow(LOCTEXT("Setting_Sensitivity", "Sensibilidade"), 0.1f, 5.0f, SensitivitySlider, SensitivityValueText, TEXT("SensitivityRow"));

	// 3. Campo de Visao / FOV (70 a 120 graus)
	AddSliderRow(LOCTEXT("Setting_FOV", "Campo de Visão (FOV)"), 70.0f, 120.0f, FOVSlider, FOVValueText, TEXT("FOVRow"));

	auto AddSelectorRow = [&](const FText& Label, TObjectPtr<UButton>& OutPrevBtn, TObjectPtr<UTextBlock>& OutValText, TObjectPtr<UButton>& OutNextBtn, FName RowName)
	{
		UVerticalBox* RowBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), RowName);

		UTextBlock* LabelBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		LabelBlock->SetText(Label);
		LabelBlock->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", MainMenuStyle::LabelFontSize));
		LabelBlock->SetColorAndOpacity(FSlateColor(MainMenuStyle::CreamTextColor));

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
		OutValText->SetColorAndOpacity(FSlateColor(MainMenuStyle::WarmSecondaryTextColor));
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

	// Botoes Aplicar e Voltar
	USizeBox* ApplySizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SettingsApplySizeBox"));
	ApplySizeBox->SetMinDesiredHeight(MainMenuStyle::ButtonMinHeight);
	SettingsApplyButton = CreateStyledButton(ApplySizeBox, LOCTEXT("Btn_Apply", "Aplicar"), MainMenuStyle::ButtonFontSize);
	ApplySizeBox->AddChild(SettingsApplyButton);

	if (UVerticalBoxSlot* ApplySlot = SettingsPanelBox->AddChildToVerticalBox(ApplySizeBox))
	{
		ApplySlot->SetHorizontalAlignment(HAlign_Fill);
		ApplySlot->SetPadding(FMargin(8.0f, 16.0f, 8.0f, 6.0f));
	}

	USizeBox* BackSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SettingsBackSizeBox"));
	BackSizeBox->SetMinDesiredHeight(MainMenuStyle::ButtonMinHeight);
	SettingsBackButton = CreateStyledButton(BackSizeBox, LOCTEXT("Btn_Back", "Voltar"), MainMenuStyle::ButtonFontSize);
	BackSizeBox->AddChild(SettingsBackButton);

	if (UVerticalBoxSlot* BackSlot = SettingsPanelBox->AddChildToVerticalBox(BackSizeBox))
	{
		BackSlot->SetHorizontalAlignment(HAlign_Fill);
		BackSlot->SetPadding(FMargin(8.0f, 4.0f));
	}
}

void UMainMenuWidget::BuildCreditsPanel()
{
	USizeBox* CardSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CreditsCardSizeBox"));
	CardSizeBox->SetWidthOverride(MainMenuStyle::SubmenuPanelWidth);
	ContentSwitcher->AddChild(CardSizeBox);

	UBorder* CardBorder = CreateSubmenuCardContainer(TEXT("CreditsCardBorder"));
	CardSizeBox->AddChild(CardBorder);

	UVerticalBox* CreditsPanelBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CreditsPanelBox"));
	CardBorder->AddChild(CreditsPanelBox);

	UTextBlock* HeaderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CreditsHeaderText"));
	HeaderText->SetText(LOCTEXT("Header_Credits", "CRÉDITOS"));
	HeaderText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", MainMenuStyle::PanelHeaderFontSize));
	HeaderText->SetColorAndOpacity(FSlateColor(MainMenuStyle::PeachAccentColor));
	HeaderText->SetJustification(ETextJustify::Center);

	if (UVerticalBoxSlot* HeaderSlot = CreditsPanelBox->AddChildToVerticalBox(HeaderText))
	{
		HeaderSlot->SetHorizontalAlignment(HAlign_Center);
		HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
	}

	// REGRA OBRIGATORIA DE CONTEUDO: "Derik Brandon" no lugar de "Derek Santos"
	UTextBlock* CreditsBody = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CreditsBodyText"));
	CreditsBody->SetText(LOCTEXT(
		"Credits_Body",
		"Desenvolvido por Sonic Studios\n\n"
		"Equipe de Desenvolvimento:\n"
		"Murilo Costa\n"
		"Mateus Favero\n"
		"Gustavo Traldi\n"
		"Bruno Sigrist\n"
		"Derik Brandon\n\n"
		"© 2026 Sonic Studios. Todos os direitos reservados.\n"
		"MOMENTUM e todas as marcas associadas são propriedade de Sonic Studios."));
	CreditsBody->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", MainMenuStyle::LabelFontSize));
	CreditsBody->SetColorAndOpacity(FSlateColor(MainMenuStyle::CreamTextColor));
	CreditsBody->SetShadowOffset(FVector2D(1.2f, 1.2f));
	CreditsBody->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
	CreditsBody->SetJustification(ETextJustify::Center);
	CreditsBody->SetAutoWrapText(true);

	if (UVerticalBoxSlot* BodySlot = CreditsPanelBox->AddChildToVerticalBox(CreditsBody))
	{
		BodySlot->SetHorizontalAlignment(HAlign_Fill);
		BodySlot->SetPadding(FMargin(6.0f, 10.0f, 6.0f, 20.0f));
	}

	USizeBox* BackSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CreditsBackSizeBox"));
	BackSizeBox->SetMinDesiredHeight(MainMenuStyle::ButtonMinHeight);
	CreditsBackButton = CreateStyledButton(BackSizeBox, LOCTEXT("Btn_Back", "Voltar"), MainMenuStyle::ButtonFontSize);
	BackSizeBox->AddChild(CreditsBackButton);

	if (UVerticalBoxSlot* BackSlot = CreditsPanelBox->AddChildToVerticalBox(BackSizeBox))
	{
		BackSlot->SetHorizontalAlignment(HAlign_Fill);
		BackSlot->SetPadding(FMargin(8.0f, 4.0f));
	}
}

void UMainMenuWidget::RefreshLevelsList()
{
	if (!LevelsListContainer || !WidgetTree)
	{
		return;
	}

	for (UButton* OldDynBtn : DynamicLevelButtons)
	{
		AllStyledButtons.Remove(OldDynBtn);
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
		EmptyText->SetColorAndOpacity(FSlateColor(MainMenuStyle::WarmSecondaryTextColor));
		EmptyText->SetJustification(ETextJustify::Center);

		if (UVerticalBoxSlot* EmptySlot = LevelsListContainer->AddChildToVerticalBox(EmptyText))
		{
			EmptySlot->SetHorizontalAlignment(HAlign_Center);
			EmptySlot->SetPadding(FMargin(0.0f, 24.0f));
		}
		return;
	}

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
			ItemSlot->SetPadding(FMargin(6.0f, 5.0f));
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
		CurrentFOV = FMath::Clamp(SaveData->FieldOfView, 70.0f, 120.0f);
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
	if (FOVSlider)
	{
		FOVSlider->SetValue(CurrentFOV);
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

	StatusMessageText->SetColorAndOpacity(FSlateColor(bIsWarning ? MainMenuStyle::PeachAccentColor : MainMenuStyle::CreamTextColor));
	StatusMessageText->SetText(Message);
	StatusMessageText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UMainMenuWidget::SwitchToPanel(int32 PanelIndex, UButton* DefaultFocusButton)
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

void UMainMenuWidget::TransitionToLevel(const TSoftObjectPtr<UWorld>& TargetLevel)
{
	// Garante bShouldShowMainMenu = false antes de carregar qualquer fase
	if (UWorld* World = GetWorld())
	{
		if (USurfGameInstance* SurfGI = Cast<USurfGameInstance>(World->GetGameInstance()))
		{
			SurfGI->bShouldShowMainMenu = false;
		}
	}

	if (APlayerController* OwningPC = GetOwningPlayer())
	{
		if (AMainMenuPlayerController* MenuPC = Cast<AMainMenuPlayerController>(OwningPC))
		{
			MenuPC->RestoreGameplayInputMode(false);
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
	// Evita execucao duplicada caso a transicao ja esteja em andamento
	if (bIsPlayingStartTransition)
	{
		return;
	}

	// 1. Define a visibilidade de Container_UI como Not Hit-Testable (Self & All Children) para evitar cliques duplos
	if (Container_UI)
	{
		Container_UI->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	// 2. Acessa a GameInstance e define bShouldShowMainMenu = False
	if (UWorld* World = GetWorld())
	{
		if (USurfGameInstance* SurfGI = Cast<USurfGameInstance>(World->GetGameInstance()))
		{
			SurfGI->bShouldShowMainMenu = false;
		}
	}

	// 3. Reproduz Anim_StartGame (0.60s: BlurStrength 25.0 -> 0.0 em 0.60s, Container_UI Opacity 1.0 -> 0.0 em 0.35s)
	bIsPlayingStartTransition = true;
	StartTransitionElapsed = 0.0f;

	if (Anim_StartGame)
	{
		PlayAnimation(Anim_StartGame, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f);
	}
}

void UMainMenuWidget::OnStartGameAnimationFinished()
{
	if (!bIsPlayingStartTransition)
	{
		return;
	}

	bIsPlayingStartTransition = false;

	if (BackgroundBlur_Main)
	{
		BackgroundBlur_Main->SetBlurStrength(0.0f);
	}
	if (Container_UI)
	{
		Container_UI->SetRenderOpacity(0.0f);
	}

	// 4. Ao finalizar Anim_StartGame (0.60s):
	//    - Obtem o PlayerController (0)
	//    - Executa Set Input Mode Game Only
	//    - Define Set Show Mouse Cursor = False
	//    - Executa Reset Ignore Input Flags (liberando a movimentacao e o olhar do jogador imediatamente)
	//    - Executa Remove from Parent no WBP_MainMenu
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		PC = UGameplayStatics::GetPlayerController(this, 0);
	}

	if (PC)
	{
		if (AMainMenuPlayerController* MenuPC = Cast<AMainMenuPlayerController>(PC))
		{
			MenuPC->RestoreGameplayInputMode();
		}
		else
		{
			FInputModeGameOnly GameInputMode;
			GameInputMode.SetConsumeCaptureMouseDown(false);
			PC->SetInputMode(GameInputMode);
			PC->bShowMouseCursor = false;
			PC->ResetIgnoreInputFlags();
		}
	}

	RemoveFromParent();
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

void UMainMenuWidget::OnDiscordClicked()
{
	FPlatformProcess::LaunchURL(TEXT("https://discord.com"), nullptr, nullptr);
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
		return;
	}

	// Ao selecionar uma fase no submenu "Fases", define bShouldShowMainMenu = False na GameInstance antes do Open Level
	if (UWorld* World = GetWorld())
	{
		if (USurfGameInstance* SurfGI = Cast<USurfGameInstance>(World->GetGameInstance()))
		{
			SurfGI->bShouldShowMainMenu = false;
		}
	}

	TransitionToLevel(Entry.Level);
}

void UMainMenuWidget::OnVolumeChanged(float NewValue)
{
	CurrentVolume = FMath::Clamp(NewValue, 0.0f, 1.0f);
	UpdateSettingsLabels();
	UMainMenuSaveGame::ApplyRealtimePreferencesToWorld(GetWorld(), CurrentVolume, CurrentSensitivity, CurrentFOV);
}

void UMainMenuWidget::OnSensitivityChanged(float NewValue)
{
	CurrentSensitivity = FMath::Clamp(NewValue, 0.1f, 5.0f);
	UpdateSettingsLabels();
	UMainMenuSaveGame::ApplyRealtimePreferencesToWorld(GetWorld(), CurrentVolume, CurrentSensitivity, CurrentFOV);
}

void UMainMenuWidget::OnFOVChanged(float NewValue)
{
	CurrentFOV = FMath::Clamp(NewValue, 70.0f, 120.0f);
	UpdateSettingsLabels();
	UMainMenuSaveGame::ApplyRealtimePreferencesToWorld(GetWorld(), CurrentVolume, CurrentSensitivity, CurrentFOV);
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
		UserSettings->ApplySettings(false);
		UserSettings->SaveSettings();
	}

	if (UMainMenuSaveGame* SaveData = UMainMenuSaveGame::LoadOrCreate())
	{
		SaveData->MasterVolume = CurrentVolume;
		SaveData->MouseSensitivity = CurrentSensitivity;
		SaveData->FieldOfView = CurrentFOV;
		SaveData->PreferredWindowMode = CurrentWindowModeIndex;
		SaveData->PreferredResolution = SelectedRes;
		SaveData->SaveToSlot();
	}

	UMainMenuSaveGame::ApplySavedPreferencesToWorld(GetWorld());

	ShowStatusMessage(LOCTEXT("Msg_SettingsSaved", "Configurações aplicadas e salvas com sucesso!"), false);
}

UButton* UMainMenuWidget::CreateStyledButton(UWidget* Outer, const FText& LabelText, int32 FontSize)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());

	auto MakeParallelogramBrush = [](const FLinearColor& FillColor) -> FSlateBrush
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(FillColor);
		Brush.OutlineSettings.CornerRadii = FVector4(
			MainMenuStyle::ButtonCornerRadius,
			MainMenuStyle::ButtonCornerRadius,
			MainMenuStyle::ButtonCornerRadius,
			MainMenuStyle::ButtonCornerRadius);
		Brush.OutlineSettings.Color = FSlateColor(MainMenuStyle::ButtonSubtleBorderColor);
		Brush.OutlineSettings.Width = MainMenuStyle::ButtonBorderThickness;
		Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		Brush.OutlineSettings.bUseBrushTransparency = true;
		return Brush;
	};

	FButtonStyle BtnStyle = Button->GetStyle();
	BtnStyle.SetNormal(MakeParallelogramBrush(MainMenuStyle::ButtonNormalBgColor));
	BtnStyle.SetHovered(MakeParallelogramBrush(MainMenuStyle::ButtonHoverBgColor));
	BtnStyle.SetPressed(MakeParallelogramBrush(MainMenuStyle::ButtonPressedBgColor));
	BtnStyle.SetNormalPadding(FMargin(22.0f, 10.0f));
	BtnStyle.SetPressedPadding(FMargin(22.0f, 11.5f, 22.0f, 8.5f));
	Button->SetStyle(BtnStyle);

	// Inclinacao diagonal estilo paralelogramo: Render Transform -> Shear X = -0.22 (-12.407418 graus)
	Button->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	Button->SetRenderShear(FVector2D(MainMenuStyle::ButtonShearAngleDeg, 0.0f));

	UTextBlock* BtnLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	BtnLabel->SetText(LabelText);

	FSlateFontInfo BtnFont = FCoreStyle::GetDefaultFontStyle("Bold", FontSize);
	BtnFont.LetterSpacing = MainMenuStyle::ButtonLetterSpacing;
	BtnLabel->SetFont(BtnFont);
	BtnLabel->SetColorAndOpacity(FSlateColor(MainMenuStyle::CreamTextColor));
	BtnLabel->SetJustification(ETextJustify::Center);

	// Compensa o Shear (Shear X = +0.22 / +12.407418 graus) no TextBlock interno para manter as letras retas
	BtnLabel->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	BtnLabel->SetRenderShear(FVector2D(-MainMenuStyle::ButtonShearAngleDeg, 0.0f));

	if (UButtonSlot* BtnSlot = Cast<UButtonSlot>(Button->AddChild(BtnLabel)))
	{
		BtnSlot->SetHorizontalAlignment(HAlign_Center);
		BtnSlot->SetVerticalAlignment(VAlign_Center);
	}

	AllStyledButtons.AddUnique(Button);
	return Button;
}

UTexture2D* UMainMenuWidget::CreateProceduralDiscordIconTexture()
{
	// Gera o icone vetorial do Discord (silhueta Clyde) 100% via SDF procedural em memoria (sem PNG externo)
	constexpr int32 Size = 64;
	UTexture2D* IconTex = UTexture2D::CreateTransient(Size, Size, PF_B8G8R8A8);
	if (!IconTex || !IconTex->GetPlatformData() || IconTex->GetPlatformData()->Mips.Num() == 0)
	{
		return nullptr;
	}

	IconTex->SRGB = true;
	IconTex->Filter = TF_Bilinear;
	IconTex->AddressX = TA_Clamp;
	IconTex->AddressY = TA_Clamp;

	FTexture2DMipMap& Mip = IconTex->GetPlatformData()->Mips[0];
	void* RawData = Mip.BulkData.Lock(LOCK_READ_WRITE);
	if (RawData)
	{
		FColor* Pixels = static_cast<FColor*>(RawData);
		for (int32 Y = 0; Y < Size; ++Y)
		{
			const float Ny = (static_cast<float>(Y) / static_cast<float>(Size - 1)) * 2.0f - 1.0f;
			for (int32 X = 0; X < Size; ++X)
			{
				const float Nx = (static_cast<float>(X) / static_cast<float>(Size - 1)) * 2.0f - 1.0f;
				const float Ax = FMath::Abs(Nx);

				// Silhueta trapezoide arredondada do controle/mascote Discord
				const float TopEdge = -0.58f + 0.14f * Ax * Ax;
				const float BottomEdge = 0.56f - 0.10f * Ax * Ax;
				const float HalfWidth = 0.78f - 0.14f * (1.0f - (Ny + 0.6f) * 0.5f);

				// Recorte inferior central entre as abas inferiores
				const bool bBottomCutout = (Ny > 0.36f) && (Ax < 0.28f) && (Ny > (0.36f + (0.28f - Ax) * 0.65f));
				// Recorte sutil no topo entre as "orelhas"
				const bool bTopNotch = (Ny < -0.44f) && (Ax < 0.24f) && (Ny < (-0.44f - (0.24f - Ax) * 0.35f));

				// Olhos circulares vazados
				const float EyeDistL = FVector2D::Distance(FVector2D(Nx, Ny), FVector2D(-0.30f, -0.02f));
				const float EyeDistR = FVector2D::Distance(FVector2D(Nx, Ny), FVector2D(0.30f, -0.02f));
				const bool bInEyes = (EyeDistL < 0.145f) || (EyeDistR < 0.145f);

				const bool bInsideBody = (Ny >= TopEdge) && (Ny <= BottomEdge) && (Ax <= HalfWidth) && !bBottomCutout && !bTopNotch && !bInEyes;

				// Anti-aliasing suave na borda externa
				float Alpha = 0.0f;
				if (bInsideBody)
				{
					const float EdgeDist = FMath::Min3(HalfWidth - Ax, Ny - TopEdge, BottomEdge - Ny);
					Alpha = FMath::Clamp(EdgeDist * 22.0f, 0.0f, 1.0f);
				}

				const uint8 A = static_cast<uint8>(FMath::RoundToInt(Alpha * 255.0f));
				Pixels[Y * Size + X] = FColor(255, 255, 255, A);
			}
		}
		Mip.BulkData.Unlock();
		IconTex->UpdateResource();
	}

	return IconTex;
}

#undef LOCTEXT_NAMESPACE
