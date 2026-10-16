#include "MainMenuGameMode.h"
#include "MainMenuPlayerController.h"

AMainMenuGameMode::AMainMenuGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = AMainMenuPlayerController::StaticClass();
	HUDClass = nullptr;
}

void AMainMenuGameMode::RestartPlayer(AController* NewPlayer)
{
	// Menu inicial nao possui Pawn; evita warning de SpawnActor sem classe.
}
