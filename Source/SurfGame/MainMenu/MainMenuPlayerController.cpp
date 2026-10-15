#include "MainMenu/MainMenuPlayerController.h"
#include "MainMenu/MainMenuWidget.h"
#include "Blueprint/UserWidget.h"

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
