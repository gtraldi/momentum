#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseMenuWidget.generated.h"

class UCanvasPanel;
class UBackgroundBlur;
class UWidgetSwitcher;
class UOverlay;
class UVerticalBox;
class UButton;
class UTextBlock;
class USlider;
class UBorder;

/**
 * Widget do Menu de Pause In-Game ("WBP_PauseMenu" / UPauseMenuWidget).
 * Reproduz a mesma identidade visual, hierarquia, fontes e estilizacao de botoes paralelogramo
 * (Shear X = -0.22, Hover Translation X = +8px, BackgroundBlur = 25.0) do WBP_MainMenu.
 *
 * Sequencia estrita de botoes:
 * 1. "Retornar" -> Despausa o jogo, oculta cursor, restaura FInputModeGameOnly e remove WBP_PauseMenu.
 * 2. "Configurações" -> Abre o painel de configuracoes com aplicacao em tempo real e persistencia na run.
 * 3. "Menu Principal" -> Despausa o jogo, define bShouldShowMainMenu = true na GameInstance e abre Mapa 1.
 * 4. "Sair" -> Executa Quit Game via PlayerController.
 */
UCLASS()
class SURFGAME_API UPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPauseMenuWidget(const FObjectInitializer& ObjectInitializer);

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** Foca no botao inicial ("Retornar") ao abrir o Pause. */
	void FocusInitialButton();

	/**
	 * Processa a tecla "Esc" quando o Pause esta aberto:
	 * - Se estiver no submenu de Configuracoes: volta para a lista principal do Pause.
	 * - Se estiver na lista principal do Pause: fecha o Pause e retorna ao jogo.
	 */
	void HandleEscapeAction();

protected:
	// Callbacks dos 4 botoes principais do Pause
	UFUNCTION()
	void OnResumeClicked();

	UFUNCTION()
	void OnSettingsClicked();

	UFUNCTION()
	void OnMainMenuClicked();

	UFUNCTION()
	void OnQuitClicked();

	UFUNCTION()
	void OnBackToPauseMainClicked();

	// Callbacks do Submenu de Configuracoes (Tempo Real + Persistencia)
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
	void OnSettingsApplyClicked();

private:
	static constexpr int32 PanelIndex_Main = 0;
	static constexpr int32 PanelIndex_Settings = 1;

	void BuildPauseMainPanel();
	void BuildSettingsPanel();

	void LoadSettingsToUI();
	void UpdateSettingsLabels();
	void SaveAndApplyRealtimeSettings(bool bApplyVideoMode);
	void ShowStatusMessage(const FText& Message, bool bIsWarning = false);
	void SwitchToPanel(int32 PanelIndex, UButton* DefaultFocusButton = nullptr);

	UButton* CreateStyledButton(UWidget* Outer, const FText& LabelText, int32 FontSize = 18);
	UBorder* CreateSubmenuCardContainer(FName CardName);

	UPROPERTY(Transient)
	TObjectPtr<UBackgroundBlur> BackgroundBlur_Pause;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> Container_UI;

	UPROPERTY(Transient)
	TObjectPtr<UOverlay> Overlay_TitleSplitHorizon;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SubtitleTextBlock;

	UPROPERTY(Transient)
	TObjectPtr<UWidgetSwitcher> ContentSwitcher;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusMessageText;

	// 4 Botoes na ordem estrita: 1. Retornar, 2. Configuracoes, 3. Menu Principal, 4. Sair
	UPROPERTY(Transient)
	TObjectPtr<UButton> ResumeButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> SettingsButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> MainMenuButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> QuitButton;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> AllStyledButtons;

	// Controles do Submenu de Configuracoes
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
	TObjectPtr<UButton> SettingsApplyButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> SettingsBackButton;

	float CurrentVolume = 1.0f;
	float CurrentSensitivity = 1.0f;
	float CurrentFOV = 90.0f;
	int32 CurrentWindowModeIndex = 1;
	int32 CurrentResolutionIndex = 2;
	TArray<FIntPoint> AvailableResolutions;
};
