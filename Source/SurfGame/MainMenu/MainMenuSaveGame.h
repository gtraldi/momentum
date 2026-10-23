#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MainMenuSaveGame.generated.h"

/**
 * SaveGame isolado do Menu Inicial para persistir volume, sensibilidade e preferencias de janela/resolucao.
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

	/** Sensibilidade do mouse salva pelo menu (nao altera o input existente nesta etapa MVP). */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Input")
	float MouseSensitivity;

	/** Modo de janela preferido (0 = Fullscreen, 1 = WindowedFullscreen, 2 = Windowed). */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Video")
	int32 PreferredWindowMode;

	/** Resolucao preferida salva no menu. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Video")
	FIntPoint PreferredResolution;

	// TODO: Adicionar rebinding de teclas e perfil/login do jogador em iteracoes futuras.

	static UMainMenuSaveGame* LoadOrCreate();
	bool SaveToSlot();

	/** Aplica o volume e a sensibilidade salvos ao World e ao PlayerInput ativos (sem alterar arquivos .ini). */
	static void ApplySavedPreferencesToWorld(UWorld* World);
};
