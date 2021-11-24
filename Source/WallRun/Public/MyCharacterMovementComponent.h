// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CustomEnums.h"
#include "MyCharacterMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class WALLRUN_API UMyCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

	friend class FMySavedMove;
	
public:

	
	
	// Speed while sprinting
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Walking")
	float SprintSpeed = 600.0f;

	// Sets sprinting to either enabled or disabled
	UFUNCTION(BlueprintCallable, Category = "Character Movement")
    void SetSprinting(bool bNewSprinting);
	
	// Experimental curve to control vertical velocity while wall runing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Wall Running")
	UCurveFloat* WallRunVerticalVelocityCurve;
	
	// Speed while wall running
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Wall Running")
	float WallRunSpeed = 800.0f;

	// Minimum speed allowed while wall running
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Wall Running")
	float MinWallRunSpeed = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Wall Running")
	float LineTraceVerticalTolerance = 50.0f;

	//Attempt to initiate a wall run
	UFUNCTION(BlueprintCallable, Category = "Custom Character Movement")
    bool StartWallRun();

	//End a wall run
	UFUNCTION(BlueprintCallable, Category = "Custom Character Movement")
    void EndWallRun();

	//Check if all required keys are down
	bool AreWallRunKeysDown() const;

	//Check if the character is next to a wall
	bool IsNextToWall(float vertical_tolerance = 0.0f);

	//Find the side of the character the wall is on and thus determine the direction to move in
	void FindWallRunDirectionAndSide(const FVector& surface_normal, FVector& direction, EWallRunSide& side) const;

	//Check if the character can wall run on said surface based on its normal
	bool CanWallRunOnSurface(const FVector& surface_normal) const;
	
	//Check if the character speed is the minimum required for wallrunning
	bool HasRequiredSpeed() const;

	//Check if the current movement mode is the desired custom mode
	bool IsDesiredCustomMovementMode(uint8 custom_movement_mode) const;

private:
	//To capture OnActorHit events from the character
	UFUNCTION()
    void OnActorHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit);

	//Used to sync the vertical velocity of the character
	float GameTimeAtStart = 0.0f;

protected:
	virtual void BeginPlay() override;
	
	//Handle on destroyed
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

public:
	//overrides
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	virtual float GetGravityZ() const override;
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
	virtual float GetMaxSpeed() const override;
	virtual void ProcessLanded(const FHitResult& Hit, float remainingTime, int32 Iterations) override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

	// custom Phys for the wallrun
	void PhysWallRunning(float deltaTime, int32 Iterations);

private:
	// Compressed flags
	uint8 bWantsToSprint : 1;
	uint8 bCanWallRun : 1;

	bool bIsSprintKeyDown = false;
	
	// The direction the character is currently wall running in
	FVector WallRunDirection;
	
	// The side of the Player the Wall is on.
	EWallRunSide WallRunSide;
};


class FMySavedMove : public FSavedMove_Character
{
public:

	typedef FSavedMove_Character Super;

	// Resets all saved variables.
	virtual void Clear() override;
	// Store input commands in the compressed flags.
	virtual uint8 GetCompressedFlags() const override;
	//network optimization to reduce number of saved moves
	virtual bool CanCombineWith(const FSavedMovePtr& NewMovePtr, ACharacter* Character, float MaxDelta) const override;
	// Sets up the move before sending it to the server. 
	virtual void SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData) override;
	// Sets variables on character movement component before making a predictive correction.
	virtual void PrepMoveFor(class ACharacter* Character) override;

private:
	uint8 bSavedWantsToSprint : 1;
	uint8 bSavedCanWallRun : 1;
};

class FMyNetworkPredictionData_Client_Character : public FNetworkPredictionData_Client_Character
{
public:
	typedef FNetworkPredictionData_Client_Character Super;

	// Constructor
	FMyNetworkPredictionData_Client_Character(const UCharacterMovementComponent& ClientMovement);

	//brief Allocates a new copy of our custom saved move
	virtual FSavedMovePtr AllocateNewMove() override;
};
