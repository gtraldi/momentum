#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/World.h"
#include "UObject/SoftObjectPtr.h"
#include "MainMenuSettings.generated.h"

USTRUCT(BlueprintType)
struct FMainMenuLevelEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level")
	TSoftObjectPtr<UWorld> Level;

	// TODO: Adicionar miniatura da fase (TSoftObjectPtr<UTexture2D> Thumbnail) e tempo recorde em iteracoes futuras.
};

/**
 * Configuracoes do Menu Inicial acessiveis em Project Settings -> Game -> Main Menu.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Main Menu"))
class SURFGAME_API UMainMenuSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UMainMenuSettings();

	/** Level aberto ao clicar no botao Jogar. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Levels")
	TSoftObjectPtr<UWorld> PlayLevel;

	/** Lista de fases exibidas no painel Fases. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Levels")
	TArray<FMainMenuLevelEntry> Levels;

	virtual FName GetCategoryName() const override;

	static const UMainMenuSettings* Get();
};
