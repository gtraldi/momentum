#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MainMenuPlayerController.generated.h"

class UMainMenuWidget;

/**
 * PlayerController dedicado ao Menu Inicial.
 * Cria o UMainMenuWidget no BeginPlay, ativa o cursor e configura FInputModeUIOnly.
 */
UCLASS()
class SURFGAME_API AMainMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMainMenuPlayerController();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMainMenuWidget> MenuWidgetInstance;
};
