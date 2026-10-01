#include "FluxMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/CapsuleComponent.h"

UFluxMovementComponent::UFluxMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Valores exatos do motor Source Engine / CS:GO
	JumpSpeed = 650.0f;      // Pulo ágil e limpo
	Gravity = 1800.0f;       // Gravidade ideal: nem flutuando na lua, nem pesada
	Friction = 3000.0f;      // Frenagem no solo plano
	MinWalkableZ = 0.7f;     // Rampa de Surf é qualquer superfície > 45.5°
	WalkSpeed = 600.0f;      // Corrida no solo
	WalkAccel = 5000.0f;     // Aceleração no solo
	AirSpeed = 30.0f;        // Teto clássico de WishSpeed no ar (Source sv_airaccelerate 150 padrão)
	AirAccel = 15000.0f;     // Alta aceleração de strafe com mouse

	bIsSurfing = false;
	LastRampNormal = FVector::UpVector;

	bWantsToJump = false;
	JumpBufferTimer = 0.0f;
	bIsOnGround = false;
}

void UFluxMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!PawnOwner || !UpdatedComponent || ShouldSkipUpdate(DeltaTime))
	{
		return;
	}

	PerformMovement(DeltaTime);
}

void UFluxMovementComponent::PerformMovement(float DeltaTime)
{
	// 1. Captura entrada direcional
	FVector Input = ConsumeInputVector();
	if (!Input.IsNearlyZero())
	{
		WishDirection = Input.GetSafeNormal();
	}
	else
	{
		WishDirection = FVector::ZeroVector;
	}

	// 2. Buffer de pulo
	if (JumpBufferTimer > 0.0f)
	{
		JumpBufferTimer -= DeltaTime;
		bWantsToJump = true;
	}
	else
	{
		bWantsToJump = false;
	}

	// 3. Verificação de solo plano
	FHitResult GroundHit;
	bIsOnGround = TryStayOnGround(GroundHit);

	if (bIsOnGround)
	{
		if (bWantsToJump)
		{
			Velocity.Z = JumpSpeed;
			bWantsToJump = false;
			JumpBufferTimer = 0.0f;
			bIsOnGround = false;
			AirMove(DeltaTime);
		}
		else
		{
			WalkMove(DeltaTime);
		}
	}
	else
	{
		AirMove(DeltaTime);
	}

	// 4. Executa movimentação cinemática e deslizamento de rampa
	MoveComponent(DeltaTime);
}

void UFluxMovementComponent::Accelerate(float DeltaTime, FVector WishDir, float WishSpeed, float Accel)
{
	if (WishDir.IsNearlyZero())
	{
		return;
	}

	// No Surf clássico do CS:
	// Aceleração só adiciona velocidade se o WishDir não estiver alinhado com a velocidade atual
	float CurrentSpeed = FVector::DotProduct(Velocity, WishDir);
	float AddSpeed = WishSpeed - CurrentSpeed;

	if (AddSpeed <= 0.0f)
	{
		return;
	}

	float AccelSpeed = Accel * DeltaTime;
	if (AccelSpeed > AddSpeed)
	{
		AccelSpeed = AddSpeed;
	}

	Velocity += WishDir * AccelSpeed;
}

void UFluxMovementComponent::ApplyFriction(float DeltaTime)
{
	float Speed = Velocity.Size();
	if (Speed <= 0.0f) return;

	float Drop = Friction * DeltaTime;
	float NewSpeed = FMath::Max(0.0f, Speed - Drop);

	Velocity = (Velocity / Speed) * NewSpeed;
}

void UFluxMovementComponent::WalkMove(float DeltaTime)
{
	ApplyFriction(DeltaTime);
	Accelerate(DeltaTime, WishDirection, WalkSpeed, WalkAccel);
	Velocity.Z = 0.0f;
}

void UFluxMovementComponent::AirMove(float DeltaTime)
{
	// No ar e na rampa: aceleração Source pura SEM freio artificial
	Accelerate(DeltaTime, WishDirection, AirSpeed, AirAccel);
	Velocity.Z -= Gravity * DeltaTime;
}

void UFluxMovementComponent::MoveComponent(float DeltaTime)
{
	float TimeLeft = DeltaTime;
	int32 MaxBumps = 4;
	bIsSurfing = false;

	for (int32 Bump = 0; Bump < MaxBumps; Bump++)
	{
		if (Velocity.IsNearlyZero() || TimeLeft <= 1.e-5f)
		{
			break;
		}

		FVector MoveDelta = Velocity * TimeLeft;
		FHitResult Hit;
		SafeMoveUpdatedComponent(MoveDelta, UpdatedComponent->GetComponentRotation(), true, Hit);

		if (Hit.bStartPenetrating)
		{
			FVector DepenDir = Hit.Normal.GetSafeNormal();
			UpdatedComponent->AddWorldOffset(DepenDir * (Hit.PenetrationDepth + 0.1f), false);
			continue;
		}

		if (Hit.IsValidBlockingHit())
		{
			TimeLeft -= TimeLeft * Hit.Time;

			FVector Normal = Hit.ImpactNormal.GetSafeNormal();

			// Detecta se é rampa de surf (entre 40° e 85° de inclinação)
			if (Normal.Z < MinWalkableZ && Normal.Z > 0.05f)
			{
				bIsSurfing = true;
				LastRampNormal = Normal;
			}

			// ClipVelocity puro (Source Engine):
			// Projeta o vetor de velocidade perpendicular à normal da face
			float Backoff = FVector::DotProduct(Velocity, Normal);
			if (Backoff < 0.0f)
			{
				Velocity = Velocity - (Normal * Backoff);
			}
		}
		else
		{
			break;
		}
	}
}

bool UFluxMovementComponent::TryStayOnGround(FHitResult& Hit)
{
	float HalfHeight = 100.0f;
	if (const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(UpdatedComponent))
	{
		HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	}

	FVector Center = UpdatedComponent->GetComponentLocation();
	FVector TraceStart = Center - FVector(0.0f, 0.0f, HalfHeight - 10.0f);
	FVector TraceEnd = Center - FVector(0.0f, 0.0f, HalfHeight + 15.0f);

	FCollisionQueryParams Params(NAME_None, false, PawnOwner);
	bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params);

	return (bHit && Hit.Normal.Z >= MinWalkableZ);
}

void UFluxMovementComponent::TryJump()
{
	bWantsToJump = true;
	JumpBufferTimer = 0.2f;
}

bool UFluxMovementComponent::IsGrounded() const
{
	return bIsOnGround;
}
