#include "MainMenuSaveGame.h"
#include "AudioDevice.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "InputCoreTypes.h"
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

void UMainMenuSaveGame::ApplySavedPreferencesToWorld(UWorld* World)
{
	if (!World)
	{
		return;
	}

	const UMainMenuSaveGame* SaveData = LoadOrCreate();
	if (!SaveData)
	{
		return;
	}

	// 1. Aplica o volume geral ao dispositivo de audio do World
	const float ClampedVolume = FMath::Clamp(SaveData->MasterVolume, 0.0f, 1.0f);
	if (FAudioDeviceHandle AudioDevice = World->GetAudioDevice())
	{
		AudioDevice->SetTransientPrimaryVolume(ClampedVolume);
	}

	// 2. Aplica a sensibilidade do mouse em runtime no PlayerInput do PlayerController ativo
	// A sensibilidade base padrao da engine em DefaultInput.ini (MouseX/MouseY/Mouse2D) e 0.07f.
	constexpr float BaseEngineMouseSensitivity = 0.07f;
	const float ClampedSensitivityMultiplier = FMath::Clamp(SaveData->MouseSensitivity, 0.1f, 5.0f);
	const float EffectiveMouseSensitivity = BaseEngineMouseSensitivity * ClampedSensitivityMultiplier;

	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		if (PC->PlayerInput)
		{
			PC->PlayerInput->SetMouseSensitivity(EffectiveMouseSensitivity, EffectiveMouseSensitivity);

			FInputAxisProperties Mouse2DProps;
			if (PC->PlayerInput->GetAxisProperties(EKeys::Mouse2D, Mouse2DProps))
			{
				Mouse2DProps.Sensitivity = EffectiveMouseSensitivity;
				PC->PlayerInput->SetAxisProperties(EKeys::Mouse2D, Mouse2DProps);
			}
		}
	}
}
