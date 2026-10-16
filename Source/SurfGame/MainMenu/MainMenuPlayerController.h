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

	/** Restaura o InputMode para GameOnly e oculta o cursor antes de abrir uma fase jogavel. */
	void RestoreGameplayInputMode();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMainMenuWidget> MenuWidgetInstance;
};
