#include "MainMenuPlayerController.h"
#include "MainMenuWidget.h"
#include "PauseMenuWidget.h"
#include "MainMenuSaveGame.h"
#include "SurfGameInstance.h"
#include "SurfCharacter.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PawnMovementComponent.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	FDelegateHandle GPostLoadMapInputRestoreHandle;
	constexpr float SeamlessSpawnBannerDuration = 2.35f;

	void GetStageBannerTextsForLevel(const FString& LevelName, FText& OutCode, FText& OutTitle, FText& OutSubtitle)
	{
		if (LevelName.Contains(TEXT("Mapa1"), ESearchCase::IgnoreCase))
		{
			OutCode = FText::FromString(TEXT("FASE 01  •  ILHA PARADISÍACA"));
			OutTitle = FText::FromString(TEXT("COSTA TROPICAL"));
			OutSubtitle = FText::FromString(TEXT("Deslize pelas brisas douradas da lagoa até a plataforma final"));
		}
		else if (LevelName.Contains(TEXT("Kitsune"), ESearchCase::IgnoreCase))
		{
			OutCode = FText::FromString(TEXT("FASE 02  •  CÉU ORIENTAL"));
			OutTitle = FText::FromString(TEXT("SANTUÁRIO KITSUNE"));
			OutSubtitle = FText::FromString(TEXT("Atravesse os arcos sagrados Torii suspensos entre as nuvens"));
		}
		else
		{
			OutCode = FText::FromString(TEXT("FASE 03  •  SETOR FUTURISTA"));
			OutTitle = FText::FromString(TEXT("ARENA CYBER NEON"));
			OutSubtitle = FText::FromString(TEXT("Domine a velocidade máxima nas rampas do circuito de neon"));
		}
	}

	void ApplyGameplayInputToWorld(UWorld* LoadedWorld)
	{
		if (!LoadedWorld)
		{
			return;
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

		// Se o Menu Principal deve ser exibido neste mundo, preserva o modo de UI
		if (const USurfGameInstance* SurfGI = Cast<USurfGameInstance>(LoadedWorld->GetGameInstance()))
		{
			const FString CurrentLevel = UGameplayStatics::GetCurrentLevelName(LoadedWorld, true);
			const bool bIsMap1 = CurrentLevel.IsEmpty() || CurrentLevel.Contains(TEXT("Mapa1"), ESearchCase::IgnoreCase);
			if (SurfGI->bShouldShowMainMenu && bIsMap1)
			{
				return;
			}
		}

		if (APlayerController* PC = LoadedWorld->GetFirstPlayerController())
		{
			FInputModeGameOnly GameInputMode;
			GameInputMode.SetConsumeCaptureMouseDown(false);
			PC->SetInputMode(GameInputMode);
			PC->bShowMouseCursor = false;
			PC->ResetIgnoreInputFlags();
		}

		if (UGameViewportClient* ViewportClient = LoadedWorld->GetGameViewport())
		{
			ViewportClient->SetIgnoreInput(false);
			ViewportClient->SetMouseLockMode(EMouseLockMode::LockOnCapture);
			ViewportClient->SetMouseCaptureMode(EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
			ViewportClient->SetHideCursorDuringCapture(true);
		}

		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().SetAllUserFocusToGameViewport();
		}
	}
}

AMainMenuPlayerController::AMainMenuPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	bShouldPerformFullTickWhenPaused = true;
	bShowMouseCursor = false;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	bAutoManageActiveCameraTarget = true;
}

void AMainMenuPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!InputComponent)
	{
		return;
	}

	// 1. Tecla "R" -> Reinicio Rapido da Fase (Quick Restart) apenas durante o gameplay ativo
	FInputKeyBinding& RestartKeyBinding = InputComponent->BindKey(EKeys::R, IE_Pressed, this, &AMainMenuPlayerController::OnQuickRestartPressed);
	RestartKeyBinding.bExecuteWhenPaused = false;

	// 2. Tecla "Esc" (e Start no Gamepad) -> Menu de Pause In-Game (Execute When Paused = True)
	FInputKeyBinding& EscKeyBinding = InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AMainMenuPlayerController::OnEscapePressed);
	EscKeyBinding.bExecuteWhenPaused = true;

	FInputKeyBinding& GamepadPauseBinding = InputComponent->BindKey(EKeys::Gamepad_Special_Right, IE_Pressed, this, &AMainMenuPlayerController::OnEscapePressed);
	GamepadPauseBinding.bExecuteWhenPaused = true;
}

bool AMainMenuPlayerController::IsMainMenuOpen() const
{
	return bWasShowingMainMenu || (MenuWidgetInstance && MenuWidgetInstance->IsInViewport());
}

bool AMainMenuPlayerController::IsPauseMenuOpen() const
{
	return bIsPauseMenuOpen || (PauseMenuWidgetInstance && PauseMenuWidgetInstance->IsInViewport());
}

void AMainMenuPlayerController::OnQuickRestartPressed()
{
	// Condicao de seguranca: a tecla "R" so deve funcionar durante o gameplay ativo
	// (ignora se o WBP_MainMenu ou o WBP_PauseMenu estiverem abertos na tela ou se o jogo estiver pausado)
	if (IsMainMenuOpen() || IsPauseMenuOpen() || IsPaused())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 1. Garante na GameInstance que bShouldShowMainMenu = False para que o Mapa 1 nao reabra o menu principal ao recarregar
	if (USurfGameInstance* SurfGI = Cast<USurfGameInstance>(World->GetGameInstance()))
	{
		SurfGI->bShouldShowMainMenu = false;
	}

	// 2. Zera imediatamente toda a velocidade/inercia antes de reiniciar a fase
	if (APawn* ControlledPawn = GetPawn())
	{
		if (UPawnMovementComponent* MoveComp = ControlledPawn->GetMovementComponent())
		{
			MoveComp->StopMovementImmediately();
			MoveComp->Velocity = FVector::ZeroVector;
		}
	}

	// 3. Reinicia a fase atual para o ponto de partida chamando GetCurrentLevelName -> OpenLevel (reset completo de estado e tempo)
	const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	if (!CurrentLevelName.IsEmpty())
	{
		UGameplayStatics::OpenLevel(this, FName(*CurrentLevelName));
	}
}

void AMainMenuPlayerController::OnEscapePressed()
{
	// Se o WBP_MainMenu estiver aberto, o "Esc" pertence apenas aos submenus do Menu Principal
	if (IsMainMenuOpen())
	{
		return;
	}

	if (IsPauseMenuOpen() && PauseMenuWidgetInstance)
	{
		// Se estiver no submenu de Configuracoes do Pause, volta para a lista principal do Pause;
		// se estiver na lista principal do Pause, fecha o Pause retornando ao jogo.
		PauseMenuWidgetInstance->HandleEscapeAction();
	}
	else
	{
		OpenPauseMenu();
	}
}

void AMainMenuPlayerController::OpenPauseMenu()
{
	if (IsMainMenuOpen() || IsPauseMenuOpen())
	{
		return;
	}

	TSubclassOf<UPauseMenuWidget> PauseClassToSpawn = UPauseMenuWidget::StaticClass();
	if (UClass* LoadedPauseWBP = LoadClass<UPauseMenuWidget>(nullptr, TEXT("/Game/MainMenu/WBP_PauseMenu.WBP_PauseMenu_C")))
	{
		PauseClassToSpawn = LoadedPauseWBP;
	}

	PauseMenuWidgetInstance = CreateWidget<UPauseMenuWidget>(this, PauseClassToSpawn);
	if (PauseMenuWidgetInstance)
	{
		bIsPauseMenuOpen = true;
		PauseMenuWidgetInstance->AddToViewport(250);

		UGameplayStatics::SetGamePaused(this, true);

		FInputModeGameAndUI PauseInputMode;
		PauseInputMode.SetWidgetToFocus(PauseMenuWidgetInstance->TakeWidget());
		PauseInputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PauseInputMode.SetHideCursorDuringCapture(false);
		SetInputMode(PauseInputMode);

		bShowMouseCursor = true;
		bEnableClickEvents = true;
		bEnableMouseOverEvents = true;
		PauseMenuWidgetInstance->FocusInitialButton();
	}
}

void AMainMenuPlayerController::ClosePauseMenu()
{
	bIsPauseMenuOpen = false;

	if (PauseMenuWidgetInstance)
	{
		PauseMenuWidgetInstance->RemoveFromParent();
		PauseMenuWidgetInstance = nullptr;
	}

	UGameplayStatics::SetGamePaused(this, false);
	RestoreGameplayInputMode(false);
}

void AMainMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	bWasShowingMainMenu = false;
	bIsPauseMenuOpen = false;
	bIsAnimatingSeamlessSpawnBanner = false;
	SeamlessBannerElapsed = 0.0f;

	if (!GPostLoadMapInputRestoreHandle.IsValid())
	{
		GPostLoadMapInputRestoreHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddStatic(&ApplyGameplayInputToWorld);
	}

	UWorld* World = GetWorld();
	if (World)
	{
		UMainMenuSaveGame::ApplySavedPreferencesToWorld(World);
	}

	// 1. Consulta a GameInstance para verificar bShouldShowMainMenu
	USurfGameInstance* SurfGI = World ? Cast<USurfGameInstance>(World->GetGameInstance()) : nullptr;
	const FString CurrentLevel = UGameplayStatics::GetCurrentLevelName(this, true);
	const bool bIsMap1 = CurrentLevel.IsEmpty() || CurrentLevel.Contains(TEXT("Mapa1"), ESearchCase::IgnoreCase);
	const bool bShouldShowMenu = (SurfGI ? SurfGI->bShouldShowMainMenu : true) && bIsMap1;

	// 2. Se bShouldShowMainMenu == False: inicia o gameplay imediatamente (mantendo o banner nativo da fase)
	if (!bShouldShowMenu)
	{
		RestoreGameplayInputMode(false);
		return;
	}

	// 3. Se bShouldShowMainMenu == True:
	bWasShowingMainMenu = true;

	// 3.1. Posiciona o Pawn/Character no PlayerStart inicial com a visao alinhada para a pista/rampa principal
	//      e trava a movimentacao e rotacao da camera (SetIgnoreMoveInput = true, SetIgnoreLookInput = true)
	AlignPawnToPlayerStartAndLockInput();
	SuppressStageHUDDuringMenu();

	if (World)
	{
		TWeakObjectPtr<AMainMenuPlayerController> WeakThis(this);
		World->GetTimerManager().SetTimerForNextTick([WeakThis]()
		{
			if (AMainMenuPlayerController* Self = WeakThis.Get())
			{
				if (Self->bWasShowingMainMenu)
				{
					Self->AlignPawnToPlayerStartAndLockInput();
					Self->SuppressStageHUDDuringMenu();
				}
			}
		});
	}

	// 3.2. Instancia o widget WBP_MainMenu e adiciona ao Viewport
	TSubclassOf<UMainMenuWidget> WidgetClassToSpawn = UMainMenuWidget::StaticClass();
	if (UClass* LoadedWBPClass = LoadClass<UMainMenuWidget>(nullptr, TEXT("/Game/MainMenu/WBP_MainMenu.WBP_MainMenu_C")))
	{
		WidgetClassToSpawn = LoadedWBPClass;
	}

	MenuWidgetInstance = CreateWidget<UMainMenuWidget>(this, WidgetClassToSpawn);
	if (MenuWidgetInstance)
	{
		MenuWidgetInstance->AddToViewport(200);

		// 3.3. Configura Set Input Mode UI Only (focado no WBP_MainMenu) e Set Show Mouse Cursor = True
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(MenuWidgetInstance->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);

		bShowMouseCursor = true;
		bEnableClickEvents = true;
		bEnableMouseOverEvents = true;
		MenuWidgetInstance->FocusInitialButton();
	}
}

void AMainMenuPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (bIsAnimatingSeamlessSpawnBanner)
	{
		SeamlessBannerElapsed += DeltaTime;

		if (ActiveStageHUDWidget)
		{
			// Mantem o veu escuro transparente apos a transicao continua e atualiza a animacao do painel tematico
			ActiveStageHUDWidget->SetFadeVeilColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
			ActiveStageHUDWidget->UpdateSpawnBannerAnimation(SeamlessBannerElapsed, SeamlessSpawnBannerDuration);
		}

		if (SeamlessBannerElapsed >= SeamlessSpawnBannerDuration)
		{
			bIsAnimatingSeamlessSpawnBanner = false;
		}
	}
}

void AMainMenuPlayerController::AlignPawnToPlayerStartAndLockInput()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	AActor* FoundStart = UGameplayStatics::GetActorOfClass(World, APlayerStart::StaticClass());
	APawn* ControlledPawn = GetPawn();

	if (ControlledPawn)
	{
		if (FoundStart)
		{
			const FVector StartLocation = FoundStart->GetActorLocation();
			const FRotator StartRotation = FoundStart->GetActorRotation();
			ControlledPawn->SetActorLocationAndRotation(StartLocation, StartRotation, false, nullptr, ETeleportType::TeleportPhysics);
			SetControlRotation(StartRotation);
		}

		// Pausa o Tick do Character e do MovementComponent enquanto o menu estiver aberto
		// (preserva o cronometro da fase em 0.00s e o estado de Spawn ate clicar em "Jogar")
		ControlledPawn->SetActorTickEnabled(false);

		if (UPawnMovementComponent* MoveComp = ControlledPawn->GetMovementComponent())
		{
			MoveComp->StopMovementImmediately();
			MoveComp->Velocity = FVector::ZeroVector;
			MoveComp->SetComponentTickEnabled(false);
		}

		SetViewTarget(ControlledPawn);
	}

	// Trava a movimentacao e a rotacao da camera enquanto o Menu Principal estiver ativo
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
}

void AMainMenuPlayerController::SuppressStageHUDDuringMenu()
{
	if (PlayerCameraManager)
	{
		PlayerCameraManager->StopCameraFade();
	}

	float TargetFOV = 90.0f;
	if (UWorld* World = GetWorld())
	{
		if (const USurfGameInstance* SurfGI = Cast<USurfGameInstance>(World->GetGameInstance()))
		{
			TargetFOV = FMath::Clamp(SurfGI->FieldOfView, 70.0f, 120.0f);
		}
	}

	if (ASurfCharacter* SurfChar = Cast<ASurfCharacter>(GetPawn()))
	{
		if (SurfChar->CameraComponent)
		{
			SurfChar->CameraComponent->SetFieldOfView(TargetFOV);
			SurfChar->CameraComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));
		}
	}

	if (UWorld* World = GetWorld())
	{
		TArray<UUserWidget*> StageHUDWidgets;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, StageHUDWidgets, USurfStageHUDWidget::StaticClass(), false);
		for (UUserWidget* Widget : StageHUDWidgets)
		{
			if (USurfStageHUDWidget* StageHUD = Cast<USurfStageHUDWidget>(Widget))
			{
				ActiveStageHUDWidget = StageHUD;
				StageHUD->HideAllBanners();
				StageHUD->SetFadeVeilColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
				StageHUD->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
}

void AMainMenuPlayerController::TriggerSeamlessStageSpawnBanner()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FString CurrentLevel = UGameplayStatics::GetCurrentLevelName(this, true);
	FText StageCode, StageTitle, StageSubtitle;
	GetStageBannerTextsForLevel(CurrentLevel, StageCode, StageTitle, StageSubtitle);

	if (ASurfCharacter* SurfChar = Cast<ASurfCharacter>(GetPawn()))
	{
		if (SurfChar->SpawnSound)
		{
			UGameplayStatics::PlaySound2D(this, SurfChar->SpawnSound, 0.42f, 1.0f);
		}
	}

	TArray<UUserWidget*> StageHUDWidgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, StageHUDWidgets, USurfStageHUDWidget::StaticClass(), false);
	for (UUserWidget* Widget : StageHUDWidgets)
	{
		if (USurfStageHUDWidget* StageHUD = Cast<USurfStageHUDWidget>(Widget))
		{
			ActiveStageHUDWidget = StageHUD;
			StageHUD->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			StageHUD->SetFadeVeilColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
			StageHUD->ApplyStageTheme(CurrentLevel);
			StageHUD->ShowSpawnBanner(StageCode, StageTitle, StageSubtitle);
			StageHUD->UpdateSpawnBannerAnimation(0.0f, SeamlessSpawnBannerDuration);

			bIsAnimatingSeamlessSpawnBanner = true;
			SeamlessBannerElapsed = 0.0f;
		}
	}
}

void AMainMenuPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsLocalController())
	{
		if (MenuWidgetInstance)
		{
			MenuWidgetInstance->RemoveFromParent();
			MenuWidgetInstance = nullptr;
		}
		if (PauseMenuWidgetInstance)
		{
			PauseMenuWidgetInstance->RemoveFromParent();
			PauseMenuWidgetInstance = nullptr;
		}
	}

	Super::EndPlay(EndPlayReason);
}

void AMainMenuPlayerController::RestoreGameplayInputMode(bool bTriggerStageBanner)
{
	if (MenuWidgetInstance)
	{
		MenuWidgetInstance->RemoveFromParent();
		MenuWidgetInstance = nullptr;
	}

	if (PauseMenuWidgetInstance)
	{
		PauseMenuWidgetInstance->RemoveFromParent();
		PauseMenuWidgetInstance = nullptr;
		bIsPauseMenuOpen = false;
	}

	if (APawn* ControlledPawn = GetPawn())
	{
		ControlledPawn->SetActorTickEnabled(true);

		if (UPawnMovementComponent* MoveComp = ControlledPawn->GetMovementComponent())
		{
			MoveComp->SetComponentTickEnabled(true);
		}
	}

	if (bWasShowingMainMenu)
	{
		bWasShowingMainMenu = false;
		if (bTriggerStageBanner)
		{
			TriggerSeamlessStageSpawnBanner();
		}
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

		UMainMenuSaveGame::ApplySavedPreferencesToWorld(World);

		TWeakObjectPtr<UWorld> WeakWorld(World);
		World->GetTimerManager().SetTimerForNextTick([WeakWorld]()
		{
			if (UWorld* ValidWorld = WeakWorld.Get())
			{
				UMainMenuSaveGame::ApplySavedPreferencesToWorld(ValidWorld);
			}
		});
	}

	FInputModeGameOnly GameInputMode;
	GameInputMode.SetConsumeCaptureMouseDown(false);
	SetInputMode(GameInputMode);

	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
	ResetIgnoreInputFlags();

	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetAllUserFocusToGameViewport();
	}
}
