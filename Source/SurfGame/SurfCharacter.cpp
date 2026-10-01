#include "SurfCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraComponent.h"
#include "FluxMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

ASurfCharacter::ASurfCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	// 1. Configuração da Cápsula (Dimensões oficiais da Source Engine)
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	CapsuleComponent->InitCapsuleSize(40.0f, 100.0f); // Raio 40cm, Meia-altura 100cm (2 metros no total)
	CapsuleComponent->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	CapsuleComponent->CanCharacterStepUpOn = ECB_No;
	RootComponent = CapsuleComponent;

	// 2. Câmera em Primeira Pessoa (Altura dos olhos a ~160cm do chão)
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(CapsuleComponent);
	CameraComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f)); // Centro da cápsula é 100 + 60 = 160cm
	CameraComponent->bUsePawnControlRotation = true;

	// 3. Componente de Movimento de Surf e Bhop
	MovementComponent = CreateDefaultSubobject<UFluxMovementComponent>(TEXT("FluxMovementComponent"));
	MovementComponent->UpdatedComponent = CapsuleComponent;

	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
}

void ASurfCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Registra o contexto do Enhanced Input se configurado
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}
}

UPawnMovementComponent* ASurfCharacter::GetMovementComponent() const
{
	return MovementComponent;
}

void ASurfCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Enhanced Input
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASurfCharacter::EnhancedMove);
		}
		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASurfCharacter::EnhancedLook);
		}
		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ASurfCharacter::OnJump);
		}
	}

	// Legacy Axis Bindings para compatibilidade direta com Project Settings
	PlayerInputComponent->BindAxis("MoveForward", this, &ASurfCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASurfCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &ASurfCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ASurfCharacter::LookUp);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ASurfCharacter::OnJump);
	PlayerInputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ASurfCharacter::OnJump);
}

void ASurfCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// No surf, a movimentação é orientada pelo Yaw do controle
		FRotator Rotation = Controller ? Controller->GetControlRotation() : GetActorRotation();
		FRotator YawRotation(0.0f, Rotation.Yaw, 0.0f);
		FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ASurfCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		FRotator Rotation = Controller ? Controller->GetControlRotation() : GetActorRotation();
		FRotator YawRotation(0.0f, Rotation.Yaw, 0.0f);
		FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, Value);
	}
}

void ASurfCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void ASurfCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void ASurfCharacter::OnJump()
{
	if (MovementComponent)
	{
		MovementComponent->TryJump();
	}
}

void ASurfCharacter::EnhancedMove(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();
	MoveForward(MovementVector.Y);
	MoveRight(MovementVector.X);
}

void ASurfCharacter::EnhancedLook(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	Turn(LookAxisVector.X);
	LookUp(-LookAxisVector.Y);
}
