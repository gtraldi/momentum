#include "MainMenuSaveGame.h"
#include "Kismet/GameplayStatics.h"

const FString UMainMenuSaveGame::SaveSlotName = TEXT("MainMenuPreferences");

UMainMenuSaveGame::UMainMenuSaveGame()
	: MasterVolume(1.0f)
	, MouseSensitivity(1.0f)
	, PreferredWindowMode(1)
	, PreferredResolution(1920, 1080)
{
}

UMainMenuSaveGame* UMainMenuSaveGame::LoadOrCreate()
{
	if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, UserIndex))
	{
		if (UMainMenuSaveGame* Loaded = Cast<UMainMenuSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, UserIndex)))
		{
			return Loaded;
		}
	}

	return Cast<UMainMenuSaveGame>(UGameplayStatics::CreateSaveGameObject(UMainMenuSaveGame::StaticClass()));
}

bool UMainMenuSaveGame::SaveToSlot()
{
	return UGameplayStatics::SaveGameToSlot(this, SaveSlotName, UserIndex);
}
