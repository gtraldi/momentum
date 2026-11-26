#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

class UCanvasPanel;
class UBackgroundBlur;
class UWidgetSwitcher;
class UOverlay;
class UVerticalBox;
class UButton;
class UTextBlock;
class USlider;
class UBorder;
class UWidgetAnimation;
class UMainMenuWidget;

/**
 * Proxy auxiliar para associar o clique de cada botao dinamico de fase ao seu indice na lista.
 */
UCLASS()
class SURFGAME_API UMainMenuLevelClickProxy : public UObject
{
	GENERATED_BODY()

public:
	void Setup(UMainMenuWidget* InOwner, int32 InLevelIndex);

	UFUNCTION()
	void HandleClicked();

private:
	UPROPERTY()
	TObjectPtr<UMainMenuWidget> OwnerWidget;

	int32 LevelIndex = INDEX_NONE;
};

/**
 * Widget principal do Menu Inicial ("WBP_MainMenu" / UMainMenuWidget) com Seamless Menu Transition sobre o Mapa 1.
 * Hierarquia:
 * - RootCanvas (CanvasPanel):
 *   1. BackgroundBlur_Main (Z-Order: 0, Full Fill 0,0->1,1, Offsets 0.0, Blur Strength = 25.0)
 *   2. Container_UI (CanvasPanel, Z-Order: 1, Full Fill 0,0->1,1, Render Opacity = 1.0):
 *      - Lado Direito / Centro-Direito: Overlay_TitleSplitHorizon "MOMENTUM" (#231817, corte horizontal deslocado com linha #E8DFD1)
 *      - Lado Esquerdo: VerticalBox (Anchor Left-Center 0.0, 0.5, Alignment 0.0, 0.5, Margin Left 100px, Padding 10px)
 *        com os 5 botoes paralelogramo (Shear X = -0.22, Hover Translation X = +8px).
 */
UCLASS()
class SURFGAME_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UMainMenuWidget(const FObjectInitializer& ObjectInitializer);

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** Foca no botao inicial (Jogar) para navegacao imediata por teclado/gamepad. */
	void FocusInitialButton();

	/** Abre uma fase da lista pelo indice configurado em UMainMenuSettings (define bShouldShowMainMenu = false antes do OpenLevel). */
	void OpenLevelByIndex(int32 Index);

	/** Widget Animation da Transicao Continua (0.60s: BlurStrength 25->0 em 0.60s, Container_UI Opacity 1->0 em 0.35s). */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "MainMenu|Animation")
	TObjectPtr<UWidgetAnimation> Anim_StartGame;

protected:
	// Callbacks do Menu Principal
	UFUNCTION()
	void OnPlayClicked();

	UFUNCTION()
	void OnStartGameAnimationFinished();

	UFUNCTION()
	void OnLevelsClicked();

	UFUNCTION()
	void OnSettingsClicked();

	UFUNCTION()
	void OnCreditsClicked();

	UFUNCTION()
	void OnQuitClicked();

	// Callbacks de retorno ao Menu Principal
	UFUNCTION()
	void OnBackToMainClicked();

	// Callbacks do Painel de Configuracoes
	UFUNCTION()
	void OnVolumeChanged(float NewValue);

	UFUNCTION()
	void OnSensitivityChanged(float NewValue);

	UFUNCTION()
	void OnFOVChanged(float NewValue);

	UFUNCTION()
	void OnWindowModePrevClicked();

	UFUNCTION()
	void OnWindowModeNextClicked();

	UFUNCTION()
	void OnResolutionPrevClicked();

	UFUNCTION()
	void OnResolutionNextClicked();

	UFUNCTION()
	void OnGraphicsQualityPrevClicked();

	UFUNCTION()
	void OnGraphicsQualityNextClicked();

	UFUNCTION()
	void OnSettingsApplyClicked();

private:
	static constexpr int32 PanelIndex_Main = 0;
	static constexpr int32 PanelIndex_Levels = 1;
	static constexpr int32 PanelIndex_Settings = 2;
	static constexpr int32 PanelIndex_Credits = 3;

	void BuildMainPanel();
	void BuildLevelsPanel();
	void BuildSettingsPanel();
	void BuildCreditsPanel();
	void BuildStartGameWidgetAnimation();

	void RefreshLevelsList();
	void LoadSettingsToUI();
	void UpdateSettingsLabels();
	void ApplyVolumeSetting(float VolumeValue);
	void ShowStatusMessage(const FText& Message, bool bIsWarning = false);
	void SwitchToPanel(int32 PanelIndex, UButton* DefaultFocusButton = nullptr);
	void TransitionToLevel(const TSoftObjectPtr<UWorld>& TargetLevel);

	UButton* CreateStyledButton(UWidget* Outer, const FText& LabelText, int32 FontSize = 18);
	UBorder* CreateSubmenuCardContainer(FName CardName);

	// Hierarquia Raiz -> BackgroundBlur_Main (Z:0) + Container_UI (Z:1)
	UPROPERTY(Transient)
	TObjectPtr<UBackgroundBlur> BackgroundBlur_Main;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> Container_UI;

	UPROPERTY(Transient)
	TObjectPtr<UOverlay> Overlay_TitleSplitHorizon;

	UPROPERTY(Transient)
	TObjectPtr<UWidgetSwitcher> ContentSwitcher;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusMessageText;

	// Botoes do painel principal (Ordem obrigatoria: Jogar, Fases, Configuracoes, Creditos, Sair)
	UPROPERTY(Transient)
	TObjectPtr<UButton> PlayButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> LevelsButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> SettingsButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CreditsButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> QuitButton;

	// Lista de todos os botoes estilizados para animacao suave de Hover (Translation X: +8px)
	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> AllStyledButtons;

	// Painel Fases
	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> LevelsListContainer;

	UPROPERTY(Transient)
	TObjectPtr<UButton> LevelsBackButton;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMainMenuLevelClickProxy>> LevelClickProxies;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> DynamicLevelButtons;

	// Painel Configuracoes
	UPROPERTY(Transient)
	TObjectPtr<USlider> VolumeSlider;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> VolumeValueText;

	UPROPERTY(Transient)
	TObjectPtr<USlider> SensitivitySlider;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SensitivityValueText;

	UPROPERTY(Transient)
	TObjectPtr<USlider> FOVSlider;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FOVValueText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> WindowModePrevButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> WindowModeNextButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> WindowModeValueText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ResolutionPrevButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ResolutionNextButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ResolutionValueText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> GraphicsQualityPrevButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> GraphicsQualityNextButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> GraphicsQualityValueText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> SettingsApplyButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> SettingsBackButton;

	// Painel Creditos
	UPROPERTY(Transient)
	TObjectPtr<UButton> CreditsBackButton;

	// Estado da Transicao Continua ("Anim_StartGame" - 0.60s)
	bool bIsPlayingStartTransition = false;
	float StartTransitionElapsed = 0.0f;

	// Estado temporario de edicao no painel Configuracoes
	float CurrentVolume = 1.0f;
	float CurrentSensitivity = 1.0f;
	float CurrentFOV = 90.0f;
	int32 CurrentWindowModeIndex = 1; // 0 = Fullscreen, 1 = WindowedFullscreen, 2 = Windowed
	int32 CurrentResolutionIndex = 2;
	int32 CurrentGraphicsQualityIndex = 2; // 0 = Baixa, 1 = Media, 2 = Alta, 3 = Epica
	TArray<FIntPoint> AvailableResolutions;
};
