#include "MainMenu/MainMenuGameMode.h"
#include "MainMenu/MainMenuPlayerController.h"

AMainMenuGameMode::AMainMenuGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = AMainMenuPlayerController::StaticClass();
	HUDClass = nullptr;
}
