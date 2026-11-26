#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SurfGameInstance.generated.h"

/**
 * GameInstance principal do projeto MOMENTUM.
 * Gerencia o estado de exibicao do Menu Inicial sobre o Mapa 1 (Seamless Menu Transition)
 * e mantem em memoria as configuracoes de gameplay/audio/video durante toda a run.
 */
UCLASS(BlueprintType, Blueprintable)
class SURFGAME_API USurfGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	USurfGameInstance();

	virtual void Init() override;

	/**
	 * Controla se o Menu Principal (WBP_MainMenu) deve ser exibido ao inicializar o Mapa 1.
	 * - Valor padrao: true (exibe o menu ao abrir o jogo).
	 * - Ao clicar em "Jogar", selecionar uma fase em "Fases" ou pressionar "R" (Quick Restart): definido como false.
	 * - Ao clicar em "Menu Principal" no WBP_PauseMenu: redefinido como true antes de carregar o Mapa 1.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MainMenu|Flow")
	bool bShouldShowMainMenu = true;

	/** Cache em memoria do Volume Geral (0.0 a 1.0) durante a run. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Audio")
	float MasterVolume = 1.0f;

	/** Cache em memoria da Sensibilidade do Mouse (0.1 a 5.0) durante a run. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Gameplay")
	float MouseSensitivity = 1.0f;

	/** Cache em memoria do Campo de Visao / FOV (70.0 a 120.0) durante a run. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Gameplay")
	float FieldOfView = 90.0f;

	/** Cache em memoria do Modo de Janela (0 = Fullscreen, 1 = WindowedFullscreen, 2 = Windowed). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Video")
	int32 PreferredWindowMode = 1;

	/** Cache em memoria da Resolucao preferida. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Video")
	FIntPoint PreferredResolution = FIntPoint(1920, 1080);

	/** Cache em memoria da Qualidade Grafica (0 = Baixa, 1 = Media, 2 = Alta, 3 = Epica). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Video")
	int32 PreferredGraphicsQuality = 2;

	/** Define se o Menu Principal deve ser exibido no proximo carregamento do Mapa 1. */
	UFUNCTION(BlueprintCallable, Category = "MainMenu|Flow")
	void SetShouldShowMainMenu(bool bInShouldShow);

	/** Consulta se o Menu Principal deve ser exibido. */
	UFUNCTION(BlueprintPure, Category = "MainMenu|Flow")
	bool ShouldShowMainMenu() const { return bShouldShowMainMenu; }

	/** Utilitario para o Menu de Pause: despausa o jogo, define bShouldShowMainMenu = true e carrega o Mapa 1. */
	UFUNCTION(BlueprintCallable, Category = "MainMenu|Flow", meta = (WorldContext = "WorldContextObject"))
	static void ReturnToMainMenuOnMap1(const UObject* WorldContextObject);
};
