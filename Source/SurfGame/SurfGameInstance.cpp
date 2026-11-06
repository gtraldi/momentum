#include "SurfGameInstance.h"
#include "MainMenu/MainMenuSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

USurfGameInstance::USurfGameInstance()
	: bShouldShowMainMenu(true)
	, MasterVolume(1.0f)
	, MouseSensitivity(1.0f)
	, FieldOfView(90.0f)
	, PreferredWindowMode(1)
	, PreferredResolution(1920, 1080)
{
}

void USurfGameInstance::Init()
{
	Super::Init();

	if (const UMainMenuSaveGame* SaveData = UMainMenuSaveGame::LoadOrCreate())
	{
		MasterVolume = FMath::Clamp(SaveData->MasterVolume, 0.0f, 1.0f);
		MouseSensitivity = FMath::Clamp(SaveData->MouseSensitivity, 0.1f, 5.0f);
		FieldOfView = FMath::Clamp(SaveData->FieldOfView, 70.0f, 120.0f);
		PreferredWindowMode = FMath::Clamp(SaveData->PreferredWindowMode, 0, 2);
		PreferredResolution = SaveData->PreferredResolution;
	}
}

void USurfGameInstance::SetShouldShowMainMenu(bool bInShouldShow)
{
	bShouldShowMainMenu = bInShouldShow;
}

void USurfGameInstance::ReturnToMainMenuOnMap1(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return;
	}

	UGameplayStatics::SetGamePaused(WorldContextObject, false);

	if (UWorld* World = WorldContextObject->GetWorld())
	{
		if (USurfGameInstance* SurfGI = Cast<USurfGameInstance>(World->GetGameInstance()))
		{
			SurfGI->bShouldShowMainMenu = true;
		}
	}

	UGameplayStatics::OpenLevel(WorldContextObject, FName(TEXT("/Game/Surf/Maps/Mapa1")));
}
