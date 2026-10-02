#include "FluxMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/CapsuleComponent.h"

UFluxMovementComponent::UFluxMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Valores exatos do motor Source Engine / CS:GO
	JumpSpeed = 650.0f;      // Pulo ágil e limpo
	Gravity = 1450.0f;       // Gravidade em queda livre amenizada (mais hangtime e voo fluido, era 1800)
	Friction = 3000.0f;      // Frenagem no solo plano
	MinWalkableZ = 0.7f;     // Rampa de Surf é qualquer superfície > 45.5°
	WalkSpeed = 600.0f;      // Corrida no solo
	WalkAccel = 5000.0f;     // Aceleração no solo
	AirSpeed = 75.0f;        // Teto de WishSpeed equilibrado: permite atingir altas velocidades sem explodir rápido
	AirAccel = 14000.0f;     // Aceleração rítmica: exige strafes contínuos e habilidosos para acumular embalo
	RampMomentumRetention = 0.92f; // Preserva 92% do momentum em subidas e curvas de rampa
	SurfGravityScale = 0.8f;       // Gravidade na rampa (1450 * 0.8 = 1160.0f para subir kickers)
	MaxSurfSpeed = 5000.0f;        // Teto máximo de velocidade (5000 u/s = altíssima velocidade, 0 = infinito)

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

	// Quando está na rampa, a gravidade é ligeiramente suavizada para permitir subir kickers sem perder todo o embalo
	float EffectiveGravity = bIsSurfing ? (Gravity * SurfGravityScale) : Gravity;
	Velocity.Z -= EffectiveGravity * DeltaTime;
}

void UFluxMovementComponent::MoveComponent(float DeltaTime)
{
	float TimeLeft = DeltaTime;
	int32 MaxBumps = 4;
	bIsSurfing = false;

	TArray<FVector, TInlineAllocator<5>> Planes;

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
			bool bHitSurfRamp = (Normal.Z < MinWalkableZ && Normal.Z > 0.05f);
			if (bHitSurfRamp)
			{
				bIsSurfing = true;
				LastRampNormal = Normal;
			}

			float SpeedBefore = Velocity.Size();

			// Guarda o plano para detecção de arestas/vincos
			Planes.Add(Normal);

			// ClipVelocity puro (Source Engine):
			float Backoff = FVector::DotProduct(Velocity, Normal);
			if (Backoff < 0.0f)
			{
				Velocity = Velocity - (Normal * Backoff);
			}

			// Se bateu em mais de um plano na mesma iteração (ex: crista/aresta do topo onde 2 faces se encontram):
			if (Planes.Num() >= 2)
			{
				FVector N1 = Planes[0];
				FVector N2 = Planes[Planes.Num() - 1];
				float PlaneDot = FVector::DotProduct(N1, N2);

				// Se as superfícies formam um ângulo agudo (aresta da crista da rampa):
				if (PlaneDot < 0.98f && PlaneDot > -0.98f)
				{
					// O produto vetorial das duas normais gera a linha exata da aresta!
					FVector CreaseDir = FVector::CrossProduct(N1, N2).GetSafeNormal();

					// Alinha a direção do vinco com o sentido de deslocamento do jogador
					if (FVector::DotProduct(CreaseDir, Velocity) < 0.0f)
					{
						CreaseDir = -CreaseDir;
					}

					// Projeta a velocidade ao longo da aresta, permitindo deslizar livremente sem prender
					Velocity = CreaseDir * FVector::DotProduct(Velocity, CreaseDir);
				}
			}

			// Em rampas de surf, compensa a perda de energia cinética causada pelas arestas das facetas poligonais
			if (bHitSurfRamp && RampMomentumRetention > 0.0f)
			{
				float SpeedAfter = Velocity.Size();
				if (SpeedAfter > 0.0f && SpeedAfter < SpeedBefore)
				{
					Velocity = Velocity * FMath::Lerp(1.0f, SpeedBefore / SpeedAfter, RampMomentumRetention);
				}
			}
		}
		else
		{
			break;
		}
	}

	// Teto máximo de velocidade para estabilidade física (se configurado > 0)
	if (MaxSurfSpeed > 0.0f && Velocity.Size() > MaxSurfSpeed)
	{
		Velocity = Velocity.GetSafeNormal() * MaxSurfSpeed;
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

	if (!bHit || Hit.Normal.Z < MinWalkableZ)
	{
		return false;
	}

	// Se o jogador estiver surfando em alta velocidade e apenas raspar numa quina ou base da rampa,
	// NÃO deve aplicar atrito de solo plano e frear instantaneamente para zero
	if (bIsSurfing && Velocity.Size() > WalkSpeed)
	{
		return false;
	}

	return true;
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
