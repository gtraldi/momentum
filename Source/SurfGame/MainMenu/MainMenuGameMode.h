#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainMenuGameMode.generated.h"

/**
 * GameMode de compatibilidade do Menu Inicial.
 * Caso o mapa legado L_MainMenu seja aberto (ex: fim de campanha na Fase 3),
 * redefine bShouldShowMainMenu = true na GameInstance e redireciona para o Mapa 1.
 */
UCLASS()
class SURFGAME_API AMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMainMenuGameMode();

	virtual void BeginPlay() override;
};
