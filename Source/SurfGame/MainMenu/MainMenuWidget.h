#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

class UCanvasPanel;
class UWidgetSwitcher;
class UVerticalBox;
class UButton;
class UTextBlock;
class USlider;
class UImage;
class UTexture2D;
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
 * Widget principal do Menu Inicial (MVP), inteiramente construido por codigo C++ (sem Widget Blueprint).
 * Contem o painel principal e os subpaineis (Fases, Config, Creditos) via UWidgetSwitcher.
 */
UCLASS()
class SURFGAME_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UMainMenuWidget(const FObjectInitializer& ObjectInitializer);

	// TODO: Permitir extensao opcional via Widget Blueprint no Designer em iteracoes futuras, se desejado.

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** Foca no botao inicial (Jogar) para navegacao imediata por teclado/gamepad. */
	void FocusInitialButton();

	/** Abre uma fase da lista pelo indice configurado em UMainMenuSettings. */
	void OpenLevelByIndex(int32 Index);

protected:
	// Callbacks do Menu Principal
	UFUNCTION()
	void OnPlayClicked();

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
	void OnWindowModePrevClicked();

	UFUNCTION()
	void OnWindowModeNextClicked();

	UFUNCTION()
	void OnResolutionPrevClicked();

	UFUNCTION()
	void OnResolutionNextClicked();

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

	void RefreshLevelsList();
	void LoadSettingsToUI();
	void UpdateSettingsLabels();
	void ApplyVolumeSetting(float VolumeValue);
	void ShowStatusMessage(const FText& Message, bool bIsWarning = false);
	void SwitchToPanel(int32 PanelIndex, UButton* DefaultFocusButton = nullptr);

	UButton* CreateStyledButton(UWidget* Outer, const FText& LabelText, int32 FontSize = 18);
	UTexture2D* CreateProceduralGradientTexture();

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> BackgroundGradientTexture;

	UPROPERTY(Transient)
	TObjectPtr<UWidgetSwitcher> ContentSwitcher;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusMessageText;

	// Botoes do painel principal
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

	// Painel Fases
	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> LevelsListContainer;

	UPROPERTY(Transient)
	TObjectPtr<UButton> LevelsBackButton;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMainMenuLevelClickProxy>> LevelClickProxies;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> DynamicLevelButtons;

	// Painel Config
	UPROPERTY(Transient)
	TObjectPtr<USlider> VolumeSlider;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> VolumeValueText;

	UPROPERTY(Transient)
	TObjectPtr<USlider> SensitivitySlider;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SensitivityValueText;

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
	TObjectPtr<UButton> SettingsApplyButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> SettingsBackButton;

	// Painel Creditos
	UPROPERTY(Transient)
	TObjectPtr<UButton> CreditsBackButton;

	// Estado temporario de edicao no painel Config
	float CurrentVolume = 1.0f;
	float CurrentSensitivity = 1.0f;
	int32 CurrentWindowModeIndex = 1; // 0 = Fullscreen, 1 = WindowedFullscreen, 2 = Windowed
	int32 CurrentResolutionIndex = 2;
	TArray<FIntPoint> AvailableResolutions;
};
