#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "SurfCharacter.generated.h"

class UCapsuleComponent;
class UCameraComponent;
class UFluxMovementComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

UCLASS()
class SURFGAME_API ASurfCharacter : public APawn
{
	GENERATED_BODY()

public:
	ASurfCharacter();

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual UPawnMovementComponent* GetMovementComponent() const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Surf | Components")
	UCapsuleComponent* CapsuleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Surf | Components")
	UCameraComponent* CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Surf | Components")
	UFluxMovementComponent* MovementComponent;

	// Enhanced Input Actions (Opcionais no Blueprint)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Surf | Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Surf | Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Surf | Input")
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Surf | Input")
	UInputAction* JumpAction;

	// Funções de Entrada
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void OnJump();

protected:
	virtual void BeginPlay() override;

	// Callbacks do Enhanced Input
	void EnhancedMove(const FInputActionValue& Value);
	void EnhancedLook(const FInputActionValue& Value);
};
