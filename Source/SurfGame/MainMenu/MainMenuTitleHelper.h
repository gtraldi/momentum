#pragma once

#include "CoreMinimal.h"

class UOverlay;
class UWidgetTree;

/**
 * Helper compartilhado para construir o logotipo "MOMENTUM" no estilo Split Horizon
 * (corte horizontal deslocado ao centro com linha de horizonte creme),
 * reutilizado por UMainMenuWidget e UPauseMenuWidget.
 */
namespace MainMenuTitleBuilder
{
	/**
	 * Constroi e retorna o Overlay_TitleSplitHorizon dentro da WidgetTree fornecida.
	 * Nao anexa ao widget pai; o chamador configura o slot no container de destino.
	 */
	UOverlay* BuildSplitHorizonTitle(UWidgetTree* WidgetTree);

	/** Converte cor hexadecimal sRGB (ex: "231817") para FLinearColor. */
	FLinearColor HexToLinear(const TCHAR* HexString, float Alpha = 1.0f);
}
