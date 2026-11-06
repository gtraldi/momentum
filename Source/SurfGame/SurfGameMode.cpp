#include "SurfGameMode.h"
#include "SurfCharacter.h"
#include "MainMenu/MainMenuPlayerController.h"

ASurfGameMode::ASurfGameMode()
{
	DefaultPawnClass = ASurfCharacter::StaticClass();
	PlayerControllerClass = AMainMenuPlayerController::StaticClass();
}
