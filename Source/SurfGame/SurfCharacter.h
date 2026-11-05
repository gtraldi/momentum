#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/UserWidget.h"
#include "SurfCharacter.generated.h"

class UCapsuleComponent;
class UCameraComponent;
class UFluxMovementComponent;
class UInputMappingContext;
class UInputAction;
class USoundBase;
class UAudioComponent;
class UCanvasPanel;
class UImage;
class UTextBlock;
class UVerticalBox;
class UHorizontalBox;
class UOverlay;
class USizeBox;
class UBorder;
class UTexture2D;
struct FInputActionValue;

UENUM(BlueprintType)
enum class ESurfTransitionState : uint8
{
	None,
	Spawning,
	FallingRespawn,
	Victory
};

/**
 * Widget de HUD e Transição de Fase construído em C++ com arte procedural, molduras e tipografia temáticas por Fase.
 */
UCLASS()
class SURFGAME_API USurfStageHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	void ApplyStageTheme(const FString& LevelName);

	void ShowSpawnBanner(const FText& StageCode, const FText& StageTitle, const FText& StageSubtitle);
	void UpdateSpawnBannerAnimation(float ElapsedTime, float TotalDuration);

	void ShowVictoryBanner(const FText& HeaderText, const FText& TitleText, const FText& TimeValueText, const FText& SpeedValueText, const FText& NextStageText);
	void UpdateVictoryBannerAnimation(float ElapsedTime, float TotalDuration);

	void ShowRespawnBanner();
	void HideAllBanners();

	void SetFadeVeilColorAndOpacity(const FLinearColor& InColor);

private:
	UTexture2D* CreateThemedPanelTexture(int32 Width, int32 Height, int32 ThemeIdx, bool bIsVictory, bool bIsAlert);
	UTexture2D* CreateThemedDividerTexture(int32 Width, int32 Height, int32 ThemeIdx, bool bIsAlert);
	UTexture2D* CreateThemedBadgeTexture(int32 Width, int32 Height, int32 ThemeIdx, bool bIsAlert);
	UTexture2D* CreateThemedStatPodTexture(int32 Width, int32 Height, int32 ThemeIdx);

	int32 CurrentThemeIndex = 0; // 0 = Tropical, 1 = Kitsune Oriental, 2 = Cyber Neon

	UPROPERTY(Transient)
	TObjectPtr<UImage> FadeVeilImage;

	// Container do Banner de Spawn / Respawn (Topo)
	UPROPERTY(Transient)
	TObjectPtr<USizeBox> SpawnBannerSizeBox;

	UPROPERTY(Transient)
	TObjectPtr<UImage> SpawnBgImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> SpawnBadgeBgImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> SpawnDividerImage;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SpawnCodeText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SpawnTitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SpawnSubtitleText;

	// Container do Card de Vitória (Centro)
	UPROPERTY(Transient)
	TObjectPtr<USizeBox> VictoryCardSizeBox;

	UPROPERTY(Transient)
	TObjectPtr<UImage> VictoryBgImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> VictoryBadgeBgImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> VictoryDividerImage;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> VictoryHeaderText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> VictoryTitleText;

	UPROPERTY(Transient)
	TObjectPtr<UImage> TimePodBgImage;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TimeStatLabelText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TimeStatValueText;

	UPROPERTY(Transient)
	TObjectPtr<UImage> SpeedPodBgImage;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SpeedStatLabelText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SpeedStatValueText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> VictoryNextStageText;

	// Texturas procedurais em cache para evitar realocação desnecessária
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> CachedSpawnPanelTex;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> CachedSpawnBadgeTex;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> CachedSpawnDividerTex;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> CachedAlertPanelTex;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> CachedAlertBadgeTex;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> CachedAlertDividerTex;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> CachedVictoryPanelTex;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> CachedVictoryBadgeTex;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> CachedVictoryDividerTex;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> CachedStatPodTex;
};

UCLASS()
class SURFGAME_API ASurfCharacter : public APawn
{
	GENERATED_BODY()

public:
	ASurfCharacter();

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual UPawnMovementComponent* GetMovementComponent() const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Surf | Components")
	UCapsuleComponent* CapsuleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Surf | Components")
	UCameraComponent* CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Surf | Components")
	UFluxMovementComponent* MovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Surf | Components")
	TObjectPtr<UAudioComponent> BackgroundMusicComponent;

	// Enhanced Input Actions (Opcionais no Blueprint)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Surf | Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Surf | Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Surf | Input")
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Surf | Input")
	UInputAction* JumpAction;

	// Efeitos Sonoros de Transição, Movimento e Música Lo-Fi por Fase
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surf | Audio")
	TObjectPtr<USoundBase> SpawnSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surf | Audio")
	TObjectPtr<USoundBase> VictorySound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surf | Audio")
	TObjectPtr<USoundBase> FallRespawnSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surf | Audio")
	TObjectPtr<USoundBase> JumpSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surf | Audio")
	TObjectPtr<USoundBase> LandSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surf | Audio")
	TObjectPtr<USoundBase> TropicalLofiMusic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surf | Audio")
	TObjectPtr<USoundBase> KitsuneLofiMusic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surf | Audio")
	TObjectPtr<USoundBase> CyberLofiMusic;

	// Funções de Entrada
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void OnJump();

protected:
	virtual void BeginPlay() override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

	// Callbacks do Enhanced Input
	void EnhancedMove(const FInputActionValue& Value);
	void EnhancedLook(const FInputActionValue& Value);

private:
	void EnsureHUDCreated();
	void StartStageBackgroundMusic();
	bool IsStandingOnFlatPlatform() const;
	void UpdateMovementAudioAndCameraFeedback(float RealDelta);
	void StartSpawnPresentation(bool bPlayBanner = true);
	void StartFallRespawnTransition();
	void StartVictoryTransition(const FString& InTargetLevel);
	FString ResolveNextLevelPath(AActor* FinishTriggerActor) const;
	void GetStageDisplayInfo(const FString& LevelName, FText& OutCode, FText& OutTitle, FText& OutSubtitle) const;
	FText GetNextStageDisplayLabel(const FString& TargetLevelPath) const;

	UPROPERTY(Transient)
	TObjectPtr<USurfStageHUDWidget> StageHUDWidget;

	ESurfTransitionState TransitionState = ESurfTransitionState::None;
	float TransitionElapsedTime = 0.0f;
	float StageRunElapsedTime = 0.0f;
	double LastRealTickTime = 0.0;

	FVector InitialSpawnLocation = FVector::ZeroVector;
	FRotator InitialSpawnRotation = FRotator::ZeroRotator;
	float DefaultCameraFOV = 90.0f;
	float DefaultCameraZ = 60.0f;

	// Controle de áudio de pulo, aterrissagem e amortecimento de câmera ao tocar o chão
	bool bWasGroundedLastFrame = true;
	bool bWasSurfingLastFrame = false;
	float AirborneTime = 0.0f;
	float SoundCooldownTimer = 0.0f;
	float LandingCameraDipOffset = 0.0f;

	FString PendingNextLevelPath;
};
