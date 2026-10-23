#include "MainMenuPlayerController.h"
#include "MainMenuWidget.h"
#include "MainMenuSaveGame.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	FDelegateHandle GPostLoadMapInputRestoreHandle;

	void ApplyGameplayInputToWorld(UWorld* LoadedWorld)
	{
		if (GPostLoadMapInputRestoreHandle.IsValid())
		{
			FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(GPostLoadMapInputRestoreHandle);
			GPostLoadMapInputRestoreHandle.Reset();
		}

		if (!LoadedWorld)
		{
			return;
		}

		if (UGameViewportClient* ViewportClient = LoadedWorld->GetGameViewport())
		{
			ViewportClient->SetIgnoreInput(false);
			ViewportClient->SetMouseLockMode(EMouseLockMode::LockOnCapture);
			ViewportClient->SetMouseCaptureMode(EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
			ViewportClient->SetHideCursorDuringCapture(true);
		}

		if (APlayerController* PC = LoadedWorld->GetFirstPlayerController())
		{
			if (!PC->IsA<AMainMenuPlayerController>())
			{
				FInputModeGameOnly GameInputMode;
				GameInputMode.SetConsumeCaptureMouseDown(false);
				PC->SetInputMode(GameInputMode);
				PC->bShowMouseCursor = false;
				PC->ResetIgnoreInputFlags();
			}
		}

		UMainMenuSaveGame::ApplySavedPreferencesToWorld(LoadedWorld);

		TWeakObjectPtr<UWorld> WeakWorld(LoadedWorld);
		LoadedWorld->GetTimerManager().SetTimerForNextTick([WeakWorld]()
		{
			if (UWorld* ValidWorld = WeakWorld.Get())
			{
				UMainMenuSaveGame::ApplySavedPreferencesToWorld(ValidWorld);
			}
		});

		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().SetAllUserFocusToGameViewport();
		}
	}
}

AMainMenuPlayerController::AMainMenuPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void AMainMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	MenuWidgetInstance = CreateWidget<UMainMenuWidget>(this, UMainMenuWidget::StaticClass());
	if (MenuWidgetInstance)
	{
		MenuWidgetInstance->AddToViewport(0);

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(MenuWidgetInstance->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);

		bShowMouseCursor = true;
		MenuWidgetInstance->FocusInitialButton();
	}
}

void AMainMenuPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsLocalController())
	{
		RestoreGameplayInputMode();
	}

	Super::EndPlay(EndPlayReason);
}

void AMainMenuPlayerController::RestoreGameplayInputMode()
{
	if (MenuWidgetInstance)
	{
		MenuWidgetInstance->RemoveFromParent();
		MenuWidgetInstance = nullptr;
	}

	if (UWorld* World = GetWorld())
	{
		if (UGameViewportClient* ViewportClient = World->GetGameViewport())
		{
			ViewportClient->SetIgnoreInput(false);
			ViewportClient->SetMouseLockMode(EMouseLockMode::LockOnCapture);
			ViewportClient->SetMouseCaptureMode(EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
			ViewportClient->SetHideCursorDuringCapture(true);
		}
	}

	FInputModeGameOnly GameInputMode;
	GameInputMode.SetConsumeCaptureMouseDown(false);
	SetInputMode(GameInputMode);

	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
	ResetIgnoreInputFlags();

	if (!GPostLoadMapInputRestoreHandle.IsValid())
	{
		GPostLoadMapInputRestoreHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddStatic(&ApplyGameplayInputToWorld);
	}

	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetAllUserFocusToGameViewport();
	}
}
