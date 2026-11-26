#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MainMenuSaveGame.generated.h"

/**
 * SaveGame do projeto MOMENTUM para persistir volume, sensibilidade, FOV e preferencias de janela/resolucao
 * entre fases, reinicios rapidos (Tecla R) e sessoes.
 */
UCLASS()
class SURFGAME_API UMainMenuSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UMainMenuSaveGame();

	static const FString SaveSlotName;
	static constexpr int32 UserIndex = 0;

	/** Volume geral (0.0 a 1.0). */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Audio")
	float MasterVolume;

	/** Multiplicador de sensibilidade do mouse (0.1 a 5.0). */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Input")
	float MouseSensitivity;

	/** Campo de visao (FOV) da camera em primeira pessoa (70.0 a 120.0, padrao 90.0). */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Gameplay")
	float FieldOfView;

	/** Modo de janela preferido (0 = Fullscreen, 1 = WindowedFullscreen, 2 = Windowed). */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Video")
	int32 PreferredWindowMode;

	/** Resolucao preferida salva no menu. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Video")
	FIntPoint PreferredResolution;

	/** Qualidade grafica geral (0 = Baixa, 1 = Media, 2 = Alta, 3 = Epica). */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Video")
	int32 PreferredGraphicsQuality;

	static UMainMenuSaveGame* LoadOrCreate();
	bool SaveToSlot();

	/** Aplica imediatamente em tempo real Volume, Sensibilidade e FOV ao World, PlayerController e Character ativos. */
	static void ApplySavedPreferencesToWorld(UWorld* World);

	/** Aplica valores especificos em tempo real ao World ativo (usado durante edicao live nos menus Principal e Pause). */
	static void ApplyRealtimePreferencesToWorld(UWorld* World, float InVolume, float InSensitivity, float InFOV);
};
