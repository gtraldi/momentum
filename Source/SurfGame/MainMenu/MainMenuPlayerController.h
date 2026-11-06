#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MainMenuPlayerController.generated.h"

class UMainMenuWidget;
class UPauseMenuWidget;
class USurfStageHUDWidget;

/**
 * PlayerController responsavel por:
 * 1. Inicializacao no Mapa 1 e Seamless Menu Transition (WBP_MainMenu).
 * 2. Reinicio Rapido da Fase (Quick Restart na tecla "R" durante gameplay ativo).
 * 3. Menu de Pause In-Game (Toggle na tecla "Esc" com bExecuteWhenPaused = true -> WBP_PauseMenu).
 * 4. Persistencia e aplicacao em tempo real de Volume, Sensibilidade, FOV e Video durante a run.
 */
UCLASS()
class SURFGAME_API AMainMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMainMenuPlayerController();

	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;

	/** Restaura o InputMode para GameOnly, oculta o cursor, destrava o movimento e opcionalmente apresenta o painel tematico da fase. */
	UFUNCTION(BlueprintCallable, Category = "MainMenu|Flow")
	void RestoreGameplayInputMode(bool bTriggerStageBanner = true);

	/** Posiciona o Pawn no PlayerStart inicial com a visao alinhada para a rampa principal e trava Move/Look input. */
	UFUNCTION(BlueprintCallable, Category = "MainMenu|Flow")
	void AlignPawnToPlayerStartAndLockInput();

	/** Reinicia rapidamente a fase atual (Tecla "R") somente se o gameplay estiver ativo (sem MainMenu ou PauseMenu abertos). */
	UFUNCTION(BlueprintCallable, Category = "Gameplay|QuickRestart")
	void OnQuickRestartPressed();

	/** Alterna o Menu de Pause In-Game (Tecla "Esc" com ExecuteWhenPaused = true). */
	UFUNCTION(BlueprintCallable, Category = "Gameplay|Pause")
	void OnEscapePressed();

	/** Abre o WBP_PauseMenu e pausa a partida. */
	UFUNCTION(BlueprintCallable, Category = "Gameplay|Pause")
	void OpenPauseMenu();

	/** Fecha o WBP_PauseMenu, despausa a partida e restaura o controle GameOnly. */
	UFUNCTION(BlueprintCallable, Category = "Gameplay|Pause")
	void ClosePauseMenu();

	/** Verifica se o Menu Principal esta aberto na tela. */
	UFUNCTION(BlueprintPure, Category = "MainMenu|Flow")
	bool IsMainMenuOpen() const;

	/** Verifica se o Menu de Pause esta aberto na tela. */
	UFUNCTION(BlueprintPure, Category = "Gameplay|Pause")
	bool IsPauseMenuOpen() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void SuppressStageHUDDuringMenu();
	void TriggerSeamlessStageSpawnBanner();

	UPROPERTY(Transient)
	TObjectPtr<UMainMenuWidget> MenuWidgetInstance;

	UPROPERTY(Transient)
	TObjectPtr<UPauseMenuWidget> PauseMenuWidgetInstance;

	UPROPERTY(Transient)
	TObjectPtr<USurfStageHUDWidget> ActiveStageHUDWidget;

	bool bWasShowingMainMenu = false;
	bool bIsPauseMenuOpen = false;
	bool bIsAnimatingSeamlessSpawnBanner = false;
	float SeamlessBannerElapsed = 0.0f;
};
