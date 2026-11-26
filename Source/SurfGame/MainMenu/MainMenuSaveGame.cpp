#include "MainMenuSaveGame.h"
#include "SurfGameInstance.h"
#include "SurfCharacter.h"
#include "AudioDevice.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "GameFramework/GameUserSettings.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"

const FString UMainMenuSaveGame::SaveSlotName = TEXT("MainMenuPreferences");

UMainMenuSaveGame::UMainMenuSaveGame()
	: MasterVolume(1.0f)
	, MouseSensitivity(1.0f)
	, FieldOfView(90.0f)
	, PreferredWindowMode(1)
	, PreferredResolution(1920, 1080)
	, PreferredGraphicsQuality(2)
{
}

UMainMenuSaveGame* UMainMenuSaveGame::LoadOrCreate()
{
	if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, UserIndex))
	{
		if (UMainMenuSaveGame* Loaded = Cast<UMainMenuSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, UserIndex)))
		{
			if (Loaded->FieldOfView < 70.0f || Loaded->FieldOfView > 120.0f)
			{
				Loaded->FieldOfView = 90.0f;
			}
			Loaded->PreferredGraphicsQuality = FMath::Clamp(Loaded->PreferredGraphicsQuality, 0, 3);
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

	if (USurfGameInstance* SurfGI = Cast<USurfGameInstance>(World->GetGameInstance()))
	{
		SurfGI->MasterVolume = FMath::Clamp(SaveData->MasterVolume, 0.0f, 1.0f);
		SurfGI->MouseSensitivity = FMath::Clamp(SaveData->MouseSensitivity, 0.1f, 5.0f);
		SurfGI->FieldOfView = FMath::Clamp(SaveData->FieldOfView, 70.0f, 120.0f);
		SurfGI->PreferredWindowMode = FMath::Clamp(SaveData->PreferredWindowMode, 0, 2);
		SurfGI->PreferredResolution = SaveData->PreferredResolution;
		SurfGI->PreferredGraphicsQuality = FMath::Clamp(SaveData->PreferredGraphicsQuality, 0, 3);
	}

	if (UGameUserSettings* UserSettings = UGameUserSettings::GetGameUserSettings())
	{
		UserSettings->SetOverallScalabilityLevel(FMath::Clamp(SaveData->PreferredGraphicsQuality, 0, 3));
		UserSettings->ApplyNonResolutionSettings();
	}

	ApplyRealtimePreferencesToWorld(World, SaveData->MasterVolume, SaveData->MouseSensitivity, SaveData->FieldOfView);
}

void UMainMenuSaveGame::ApplyRealtimePreferencesToWorld(UWorld* World, float InVolume, float InSensitivity, float InFOV)
{
	if (!World)
	{
		return;
	}

	// 1. Aplica o volume geral em tempo real ao dispositivo de audio do World
	const float ClampedVolume = FMath::Clamp(InVolume, 0.0f, 1.0f);
	if (FAudioDeviceHandle AudioDevice = World->GetAudioDevice())
	{
		AudioDevice->SetTransientPrimaryVolume(ClampedVolume);
	}

	// 2. Aplica a sensibilidade do mouse em tempo real no PlayerInput do PlayerController ativo
	// A sensibilidade base padrao da engine em DefaultInput.ini (MouseX/MouseY/Mouse2D) e 0.07f.
	constexpr float BaseEngineMouseSensitivity = 0.07f;
	const float ClampedSensitivityMultiplier = FMath::Clamp(InSensitivity, 0.1f, 5.0f);
	const float EffectiveMouseSensitivity = BaseEngineMouseSensitivity * ClampedSensitivityMultiplier;

	// 3. Aplica o FOV (Campo de Visao) em tempo real na CameraComponent do Character ativo
	const float ClampedFOV = FMath::Clamp(InFOV, 70.0f, 120.0f);

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

		if (ASurfCharacter* SurfChar = Cast<ASurfCharacter>(PC->GetPawn()))
		{
			if (SurfChar->CameraComponent)
			{
				SurfChar->CameraComponent->SetFieldOfView(ClampedFOV);
			}
		}
	}
}
