#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainMenuGameMode.generated.h"

/**
 * GameMode isolado do Menu Inicial (sem Pawn, sem HUD e usando AMainMenuPlayerController).
 */
UCLASS()
class SURFGAME_API AMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMainMenuGameMode();
};
