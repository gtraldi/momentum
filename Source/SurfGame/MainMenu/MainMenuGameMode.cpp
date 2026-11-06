#include "MainMenuGameMode.h"
#include "MainMenuPlayerController.h"
#include "SurfCharacter.h"
#include "SurfGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

AMainMenuGameMode::AMainMenuGameMode()
{
	DefaultPawnClass = ASurfCharacter::StaticClass();
	PlayerControllerClass = AMainMenuPlayerController::StaticClass();
	HUDClass = nullptr;
}

void AMainMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (USurfGameInstance* SurfGI = Cast<USurfGameInstance>(World->GetGameInstance()))
		{
			SurfGI->bShouldShowMainMenu = true;
		}

		const FString CurrentLevel = UGameplayStatics::GetCurrentLevelName(this, true);
		if (!CurrentLevel.Equals(TEXT("Mapa1"), ESearchCase::IgnoreCase))
		{
			UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/Surf/Maps/Mapa1")));
		}
	}
}
