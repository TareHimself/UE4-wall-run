// Fill out your copyright notice in the Description page of Project Settings.


#include "MyCharacterMovementComponent.h"



#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/PlayerInput.h"
#include "Kismet/KismetStringLibrary.h"
#include "Kismet/KismetStringTableLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Math/UnitConversion.h"

void UMyCharacterMovementComponent::SetSprinting(bool bNewSprinting)
{
	bIsSprintKeyDown = bNewSprinting;
}

bool UMyCharacterMovementComponent::StartWallRun()
{
	// check if we can wallrun
	if (bCanWallRun == true)
	{
		// Set the movement mode to wall running.
		SetMovementMode(EMovementMode::MOVE_Custom, ECustomMovementMode::CMOVE_WallRunning);
		return true;
	}
	
	return false;
}

void UMyCharacterMovementComponent::EndWallRun()
{
	// Set the movement mode back to falling
	SetMovementMode(EMovementMode::MOVE_Falling);
}

bool UMyCharacterMovementComponent::AreWallRunKeysDown() const
{
	// ensure this only happens locally
	if (!GetPawnOwner()->IsLocallyControlled())
		return false;

	// Ensure the required key (sprint) is down
	TArray<FInputActionKeyMapping> sprintKeyMappings;
	UInputSettings::GetInputSettings()->GetActionMappingByName("Sprint", sprintKeyMappings);
	for (FInputActionKeyMapping& sprintKeyMapping : sprintKeyMappings)
	{
		if (GetPawnOwner()->GetController<APlayerController>()->IsInputKeyDown(sprintKeyMapping.Key))
		{
			return true;
		}
	}

	return false;
}

bool UMyCharacterMovementComponent::IsNextToWall(float vertical_tolerance)
{
	// Do a line trace from the player into the wall to make sure we're stil along the side of a wall
	FVector crossVector = WallRunSide == EWallRunSide::Side_Left ? FVector(0.0f, 0.0f, -1.0f) : FVector(0.0f, 0.0f, 1.0f);
	FVector traceStart = GetPawnOwner()->GetActorLocation() + (WallRunDirection * 20.0f);
	FVector traceEnd = traceStart + (FVector::CrossProduct(WallRunDirection, crossVector) * 100);
	FHitResult hitResult;

	// Create a helper lambda for performing the line trace
	auto lineTrace = [&](const FVector& start, const FVector& end)
	{
		return (GetWorld()->LineTraceSingleByChannel(hitResult, start, end, ECollisionChannel::ECC_Visibility));
	};

	// If a vertical tolerance was provided we want to do two line traces - one above and one below the calculated line
	if (vertical_tolerance > FLT_EPSILON)
	{
		// If both line traces miss the wall then return false, we're not next to a wall
		if (lineTrace(FVector(traceStart.X, traceStart.Y, traceStart.Z + vertical_tolerance / 2.0f), FVector(traceEnd.X, traceEnd.Y, traceEnd.Z + vertical_tolerance / 2.0f)) == false &&
            lineTrace(FVector(traceStart.X, traceStart.Y, traceStart.Z - vertical_tolerance / 2.0f), FVector(traceEnd.X, traceEnd.Y, traceEnd.Z - vertical_tolerance / 2.0f)) == false)
		{
			return false;
		}
	}
	// If no vertical tolerance was provided we just want to do one line trace using the caclulated line
	else
	{
		// return false if the line trace misses the wall
		if (lineTrace(traceStart, traceEnd) == false)
			return false;
	}

	// Make sure the wall is still on the expected side of the charactr
	EWallRunSide newWallRunSide;
	FindWallRunDirectionAndSide(hitResult.ImpactNormal, WallRunDirection, newWallRunSide);
	if (newWallRunSide != WallRunSide)
	{
		return false;
	}

	return true;
}

void UMyCharacterMovementComponent::FindWallRunDirectionAndSide(const FVector& surface_normal, FVector& direction,
	EWallRunSide& side) const
{
	FVector crossVector;

	if (FVector2D::DotProduct(FVector2D(surface_normal), FVector2D(GetPawnOwner()->GetActorRightVector())) > 0.0)
	{
		side = EWallRunSide::Side_Right;
		crossVector = FVector(0.0f, 0.0f, 1.0f);
	}
	else
	{
		side = EWallRunSide::Side_Left;
		crossVector = FVector(0.0f, 0.0f, -1.0f);
	}

	// Find the direction parallel to the wall in the direction the player is moving
	direction = FVector::CrossProduct(surface_normal, crossVector);
}

bool UMyCharacterMovementComponent::CanWallRunOnSurface(const FVector& surface_normal) const
{
	// check if the surface normal is facing down
	if (surface_normal.Z < -0.05f)
		return false;

	FVector normalNoZ = FVector(surface_normal.X, surface_normal.Y, 0.0f);
	normalNoZ.Normalize();

	// Find the angle of the wall
	const float wallAngle = FMath::Acos(FVector::DotProduct(normalNoZ, surface_normal));

	// Return true if the wall angle is less than the walkable floor angle
	return wallAngle < GetWalkableFloorAngle();
}

bool UMyCharacterMovementComponent::HasRequiredSpeed() const
{
	// Use the linear physics velocity of the capsule component
	const float CurrentPhysicsVelocity = (GetCharacterOwner()->GetCapsuleComponent()->GetPhysicsLinearVelocity() * FVector(1.0f,1.0f,0.0f)).Size();

	// return false if its less that our specified MinWallRunSpeed
	return CurrentPhysicsVelocity > MinWallRunSpeed;

}

bool UMyCharacterMovementComponent::IsDesiredCustomMovementMode(uint8 custom_movement_mode) const
{
	return MovementMode == EMovementMode::MOVE_Custom && CustomMovementMode == custom_movement_mode;
}

void UMyCharacterMovementComponent::OnActorHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse,
	const FHitResult& Hit)
{
	// check if we're already wall running
	if (IsDesiredCustomMovementMode(ECustomMovementMode::CMOVE_WallRunning))
		return;

	// check if we falling
	if (!IsFalling())
		return;

	// check if we can wall run on the surface we hit
	if (!CanWallRunOnSurface(Hit.ImpactNormal))
		return;
	
	FindWallRunDirectionAndSide(Hit.ImpactNormal, WallRunDirection, WallRunSide);

	// check if we're next to a wall
	if (!IsNextToWall())
		return;

	StartWallRun();
}

void UMyCharacterMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	// bind the OnActorHit delegate to our OnActorHit function on all Net roles greater than ROLE_SimulatedProxy
	if (GetPawnOwner()->GetLocalRole() > ROLE_SimulatedProxy)
	{
		GetPawnOwner()->OnActorHit.AddDynamic(this, &UMyCharacterMovementComponent::OnActorHit);
	}
}

void UMyCharacterMovementComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	// unbind the OnActorHit delegate to our OnActorHit function on all Net roles greater than ROLE_SimulatedProxy
	if (GetPawnOwner() != nullptr && GetPawnOwner()->GetLocalRole() > ROLE_SimulatedProxy)
	{
		GetPawnOwner()->OnActorHit.RemoveDynamic(this, &UMyCharacterMovementComponent::OnActorHit);
	}

	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UMyCharacterMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	if (GetPawnOwner()->IsLocallyControlled())
	{
		// sprinting checks
		if (bIsSprintKeyDown)
		{
			FVector velocity2D = GetPawnOwner()->GetVelocity();
			FVector forward2D = GetPawnOwner()->GetActorForwardVector();
			velocity2D.Z = 0.0f;
			forward2D.Z = 0.0f;
			velocity2D.Normalize();
			forward2D.Normalize();

			bWantsToSprint = FVector::DotProduct(velocity2D, forward2D) > 0.5f;
		}
		else
		{
			bWantsToSprint = false;
		}

		// CanwWllRun checks
		bCanWallRun = AreWallRunKeysDown() && HasRequiredSpeed();
	}

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UMyCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);
	
	bWantsToSprint = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
	bCanWallRun = (Flags & FSavedMove_Character::FLAG_Custom_1) != 0;
}

void UMyCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
	if(MovementMode == MOVE_Custom)
	{
		if(CustomMovementMode == ECustomMovementMode::CMOVE_WallRunning)
		{
			// Store the game time when we started wallrunning
			GameTimeAtStart = UKismetSystemLibrary::GetGameTimeInSeconds(GetOwner());
		}
	}
}

float UMyCharacterMovementComponent::GetGravityZ() const
{
	// Remove gravity while wall running (custom gravity handled by float curve)
	if(IsDesiredCustomMovementMode(ECustomMovementMode::CMOVE_WallRunning))
	{
		return 0.0f;
	}
	
	return Super::GetGravityZ();
}

void UMyCharacterMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	// ensure this is never done on the simulated proxy
	if(GetOwnerRole() == ROLE_SimulatedProxy)
	{
		return;
	}

	// call desired custom Phys implementation
	switch (CustomMovementMode)
	{
		case ECustomMovementMode::CMOVE_WallRunning:
			PhysWallRunning(deltaTime,Iterations);
			break;
	}

	// Just in case
	Super::PhysCustom(deltaTime,Iterations);
}

float UMyCharacterMovementComponent::GetMaxSpeed() const
{
	// override max walk speed for sprinting purposes
	if(MovementMode == EMovementMode::MOVE_Walking || MovementMode == EMovementMode::MOVE_NavWalking )
	{
		if(IsCrouching())
		{
			return MaxWalkSpeedCrouched;
		}
		else
		{
			if(bWantsToSprint)
			{
				return SprintSpeed;
			}
			else
			{
				return MaxWalkSpeed;
			}
		}
	}
	
	return Super::GetMaxSpeed();
}


void UMyCharacterMovementComponent::ProcessLanded(const FHitResult& Hit, float remainingTime, int32 Iterations)
{
	// only do if we are Wall Running
	if (IsDesiredCustomMovementMode(ECustomMovementMode::CMOVE_WallRunning))
	{
		EndWallRun();
	}
	
	Super::ProcessLanded(Hit, remainingTime, Iterations);

	// to make sure we can jump normally after
	GetCharacterOwner()->ResetJumpState();
}

FNetworkPredictionData_Client* UMyCharacterMovementComponent::GetPredictionData_Client() const
{
	if (ClientPredictionData == nullptr)
	{
		// Return our custom client prediction class instead
		UMyCharacterMovementComponent* MutableThis = const_cast<UMyCharacterMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FMyNetworkPredictionData_Client_Character(*this);
	}

	return ClientPredictionData;
}

void UMyCharacterMovementComponent::PhysWallRunning(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	// check if we can still wall run
	if (!bCanWallRun)
	{
		EndWallRun();
		return;
	}

	// check if we are stil next to a wall
	if (!IsNextToWall(LineTraceVerticalTolerance))
	{
		EndWallRun();
		return;
	}

	//find the time elapsed since we started wall running
	const float TimeSinceStart = UKismetSystemLibrary::GetGameTimeInSeconds(GetOwner()) - GameTimeAtStart;
	float ZVelocity = 0.0f;

	// Get the Z velocity from the float curve if its valid
	if(WallRunVerticalVelocityCurve)
	{
		ZVelocity = WallRunVerticalVelocityCurve->GetFloatValue(TimeSinceStart);
	}

	FVector characterVelocity = WallRunDirection;
	characterVelocity.X *= WallRunSpeed;
	characterVelocity.Y *= WallRunSpeed;
	characterVelocity.Z = ZVelocity;
	Velocity = characterVelocity;

	const FVector Adjusted = Velocity * deltaTime;
	FHitResult Hit(1.f);
	SafeMoveUpdatedComponent(Adjusted, UpdatedComponent->GetComponentQuat(), true, Hit);

}

void FMySavedMove::Clear()
{
	Super::Clear();

	// Clear all values
	bSavedWantsToSprint = 0;
	bSavedCanWallRun = 0;
}

uint8 FMySavedMove::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();

	if (bSavedWantsToSprint)
	{
		Result |= FLAG_Custom_0;
	}
		
	if (bSavedCanWallRun)
	{
		Result |= FLAG_Custom_1;
	}

	return Result;
}

bool FMySavedMove::CanCombineWith(const FSavedMovePtr& NewMovePtr, ACharacter* Character, float MaxDelta) const
{
	const FMySavedMove* NewMove = static_cast<const FMySavedMove*>(NewMovePtr.Get());

	if (bSavedWantsToSprint != NewMove->bSavedWantsToSprint ||
        bSavedCanWallRun != NewMove->bSavedCanWallRun)
	{
		return false;
	}

	return Super::CanCombineWith(NewMovePtr, Character, MaxDelta);
}

void FMySavedMove::SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel,
	FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);

	UMyCharacterMovementComponent* MovementComponent = static_cast<UMyCharacterMovementComponent*>(Character->GetCharacterMovement());
	if (MovementComponent)
	{
		bSavedWantsToSprint = MovementComponent->bWantsToSprint;
		bSavedCanWallRun = MovementComponent->bCanWallRun;
	}
}

void FMySavedMove::PrepMoveFor(ACharacter* Character)
{
	Super::PrepMoveFor(Character);

	UMyCharacterMovementComponent* MovementComponent = Cast<UMyCharacterMovementComponent>(Character->GetCharacterMovement());
	if (MovementComponent)
	{
		MovementComponent->bWantsToSprint = bSavedWantsToSprint;
		MovementComponent->bCanWallRun = bSavedCanWallRun;
	}
}

FMyNetworkPredictionData_Client_Character::FMyNetworkPredictionData_Client_Character(
	const UCharacterMovementComponent& ClientMovement) : Super(ClientMovement)
{
	
}

FSavedMovePtr FMyNetworkPredictionData_Client_Character::AllocateNewMove()
{
	return FSavedMovePtr(new FMySavedMove());
}
