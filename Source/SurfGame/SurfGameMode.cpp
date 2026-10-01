#include "SurfGameMode.h"
#include "SurfCharacter.h"

ASurfGameMode::ASurfGameMode()
{
	DefaultPawnClass = ASurfCharacter::StaticClass();
}
