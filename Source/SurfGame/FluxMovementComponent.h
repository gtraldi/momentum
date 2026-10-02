#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "FluxMovementComponent.generated.h"

/**
 * Componente de Movimento de Surf e Bhop para Unreal Engine 5
 * Implementação fiel da física da Source Engine (CS:GO / CS:S / Quake).
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SURFGAME_API UFluxMovementComponent : public UPawnMovementComponent
{
	GENERATED_BODY()

public:
	UFluxMovementComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// --- PARÂMETROS DE FÍSICA ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surf | Physics")
	float JumpSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surf | Physics")
	float Gravity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surf | Physics")
	float Friction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surf | Physics")
	float MinWalkableZ;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surf | Physics")
	float WalkSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surf | Physics")
	float WalkAccel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surf | Physics")
	float AirSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surf | Physics")
	float AirAccel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surf | Physics")
	float RampMomentumRetention;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surf | Physics")
	float SurfGravityScale;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Surf | Status")
	bool bIsSurfing;

	// --- FUNÇÕES PÚBLICAS ---

	UFUNCTION(BlueprintCallable, Category="Surf | Actions")
	void TryJump();

	UFUNCTION(BlueprintPure, Category="Surf | Status")
	bool IsGrounded() const;

	UFUNCTION(BlueprintPure, Category="Surf | Status")
	bool IsSurfing() const { return bIsSurfing; }

private:
	void PerformMovement(float DeltaTime);
	void WalkMove(float DeltaTime);
	void AirMove(float DeltaTime);
	void Accelerate(float DeltaTime, FVector WishDir, float WishSpeed, float Accel);
	void ApplyFriction(float DeltaTime);
	void MoveComponent(float DeltaTime);
	bool TryStayOnGround(FHitResult& Hit);

	FVector WishDirection;
	FVector LastRampNormal;
	bool bWantsToJump;
	float JumpBufferTimer;
	bool bIsOnGround;
};
