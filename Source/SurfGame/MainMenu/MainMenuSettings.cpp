#include "MainMenu/MainMenuSettings.h"

UMainMenuSettings::UMainMenuSettings()
{
}

FName UMainMenuSettings::GetCategoryName() const
{
	return FName(TEXT("Game"));
}

const UMainMenuSettings* UMainMenuSettings::Get()
{
	return GetDefault<UMainMenuSettings>();
}
